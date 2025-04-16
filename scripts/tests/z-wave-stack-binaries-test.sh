#! /usr/bin/env bash
# -*- mode: Bash; tab-width: 2; indent-tabs-mode: nil; coding: utf-8 -*-
# vim:shiftwidth=4:softtabstop=4:tabstop=4:
#
#% * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * %#

[ "" = "$debug" ] || set
[ "" = "$debug" ] || set -x
set -e
set -o pipefail

# Default configuration can be overloaded from env

duration=10 # Allocated time in mins until watchdog quit

ZPC_COMMAND="${ZPC_COMMAND:=/usr/bin/zpc}"
zpc_api="${zpc_api:=uic}"

[ "" != "${z_wave_stack_binaries_bin_dir}" ] \
    || z_wave_stack_binaries_bin_dir="${PWD}/z-wave-stack-binaries/bin"

# Internal

name='z-wave'
code=0
code_log="code.log.tmp"
zpc_log="zpc.log.tmp"
file="screen.rc"
mqtt_pub_log="mqtt_pub.log.tmp"
mqtt_sub_log="mqtt_sub.log.tmp"
mqtt_log="mqtt.log.tmp"
controller_log="controller.log.tmp"
node_log="node.log.tmp"

nocol="\e[0m"
blue="\e[0;34m"
cyan='\e[1;36m'
green="\e[0;32m"
red="\e[0;31m"
yellow='\e[1;33m'


usage_()
{
    cat<<EOF
Usage:

  $0 setup_ # Download dependencies
  $0 # Run installed version

Shortcuts:

  Ctrl+a \ : Quit all windows
  Ctrl+a TAB : change focus
  Ctrl+a ESC : Take focus to use scroll keys

Advanced uses:

Run from docker:

export ZPC_COMMAND="docker-compose up --abort-on-container-exit"
export ZPC_ARGS="--log.level=d"

docker-compose down # Disable local broker (will start a new one)
pidof mosquitto && sudo systemctl stop mosquitto

$0

Run from prebuilt env:

export z_wave_stack_binaries_bin_dir="${HOME}/mnt/z-wave-stack/build-Debug/_CPack_Packages/Linux/TGZ/z-wave-stack-0.0.0-Linux/bin"

export ZPC_COMMAND="$PWD/build/applications/zpc/zpc --mapdir=applications/zpc/components/dotdot_mapper/rules --zpc.datastore_file=tmp.db --zpc.ota.cache_path="tmp" --log.level=d"

sudo systemctl stop mosquitto ; sudo rm /var/lib/mosquitto/mosquitto.db ; sudo systemctl restart mosquitto ;
rm -rfv *.tmp
./scripts/tests/z-wave-stack-binaries-test.sh

More:

This tool depends on:
https://github.com/Z-Wave-Alliance/z-wave-stack-binaries


For debuging:
debug=1 $0

EOF
}


log_()
{
    [ "$debug" != "" ] && echo || clear
    printf "${yellow}info: $@ ${nocol}\n"
}


exit_()
{
    local code=0$1
    log_ "exit: $@ code=$code ; Use: debug=1 $0 # To trace script)"
    sleep 10
    [ -z $debug ] || sleep 1000
    echo $code > $code_log
    screen -S "$name" -X "quit"
    exit $code
}


die_()
{
    local code=$(( 1 + 0$code))
    exit_ $code
}


run_()
{
    local task="$1" && shift
    local log="$task.log.tmp"
    rm -f "$log"
    ${task}_ "$@" 2>&1 | tee "$log"
}


mqtt_()
{
    log_ "mqtt: Wait for broker then flush queue before use and log traffic"
    while true ; do
        pidof mosquitto \
            && mosquitto_sub \
                   -v -t '#' --remove-retained --retained-only -W 1 \
            && break
        sleep .1
    done > /dev/null 2>&1
    mosquitto_sub -v -t '#' -W $((60 * ${duration}))
    log_ "mqtt: error: Should have finish before ${duration} may need to update it"
    die_
}

sub_()
{
    local sub="#"
    local count=1
    local expect=""

    [ "$1" = "" ] || sub="$1"
    [ "$2" = "" ] || expect="$2"
    [ "$3" = "" ] || count="$3"

    printf "${cyan}sub: $sub${nocol} (count=${count})\n"
    mosquitto_sub -v -t "$sub" -C $count | tee "$mqtt_sub_log"
    [ "" = "$expect" ] || printf "${blue}exp: $expect${nocol}\n"

    if [ "" != "$expect" ] ; then
        tail -n +$count "$mqtt_sub_log" \
            | head -n1 | grep -F "$expect" 2>&1 > /dev/null \
            || { printf "${red}exp: error:: ${expect}${nocol}\n" ;
                 cat "$mqtt_sub_log"  ;
                 die_ ; }
    fi
}


pub_()
{
    topic="$1"
    message="$2"
    log_ "pub: $@"

    [ "" = "$message" ] || printf "$message" | jq
    sleep 1
    mosquitto_pub -t "$topic" -m "$message"
    sleep 1
}


pubsub_()
{
    # reset
    echo ""
    [ "" = "$debug" ] || log_ "pubsub_: $@"
    local pub="$1"
    local message="$2"
    local sub="$pub"
    local expect="$sub"
    local delay=1
    local count=1

    [ "$3" = "" ] || sub="$3"
    [ "$4" = "" ] || expect="$4"
    [ "$5" = "" ] || count="$5"
    [ "$6" = "" ] || delay="$6"

    if [ "" != "$pub" ] ; then
        printf "${green}pub: $pub${nocol}\n"
        if [ "$message" != "" ] ; then
            printf "$message" | jq --color-output
        fi
    fi
    if [ "" != "$pub" ] ; then
        mosquitto_pub -t "$pub" -m "$message" | tee "$mqtt_pub_log"
    fi
    sleep $delay # Needed to dedup
    if [ "" != "$sub" ] ; then
        sub_ "$sub" "$expect" "$count"
    fi
    sleep 1
}


controller_()
{
    controller_app=$(realpath "${z_wave_stack_binaries_bin_dir}/ZW_zwave_ncp_serial_api_controller_"*"_REALTIME"*".elf" | head -n1)
    file -E "${controller_app}"
    ${controller_app} --pty
}


controller_cli_()
{
    while [ ! -e "${controller_log}" ] ; do sleep 1; done
    sleep 1
    screen -S "$name" -p 1 -t "controller" -X stuff "$@^M"
    sleep 1
    case $1 in
        h)
            echo "controller: Should display help"
            ;;

        p)
            PTY=$(grep 'PTY: ' ${controller_log} \
                      | tail -n 1 | sed -e 's|PTY: \(.*\)|\1|g' )
            ;;
        H)
            [ -z $debug ] || log_ "TODO: print HOME_ID: from device: https://github.com/Z-Wave-Alliance/z-wave-stack/issues/732"
            sleep 1
            while [ ! -e "${mqtt_log}" ] ; do sleep 1; done
            # [ "$homeid" != "" ]
            homeid=$(sed -n -e 's|ucl/by-unid/zw-\(.*\)-\([0-9]*\)/.*|\1|gp' "$mqtt_log" | tail -n1)
            echo "HOME_ID: ${homeid}"
            [ ! -z $homeid ] || die_
            ;;
        n)
            contid=$(grep 'NODE_ID: ' "${controller_log}" \
                         | tail -n 1 | sed -e 's|NODE_ID: \(.*\)|\1|g' )
            echo "NODE_ID: ${contid}"
            contid=$(printf "%04d" $contid)
            contunid="zw-$homeid-$contid"
            echo "NODE_UNID: ${contunid}"
            ;;
        *)
            echo "TODO: handle $1"
            ;;
    esac
}


node_()
{
    node_app=$(realpath "${z_wave_stack_binaries_bin_dir}/ZW_zwave_soc_switch_on_off_"*"_REALTIME"*".elf" | head -n1)
    [ "$debug" = "" ] || file -E "${node_app}"
    ${node_app} --pty
}


node_cli_()
{
    while [ ! -e "${node_log}" ] ; do sleep 1; done
    screen -S "$name" -p 2 -t "node" -X stuff "$@^M"
    sleep 1
    case $1 in
        h)
            echo "node: Should display help"
            ;;
        d)
            DSK=$(grep 'DSK: ' "${node_log}" \
                      | tail -n 1 | sed -e 's|DSK: \([0-9-]*\)|\1|g' )
            SecurityCode=$(echo "$DSK" | sed -e 's|\([0-9]*\)-[0-9-]*$|\1|g')
            ;;
        l)
            echo "node: Set to learn mode ${nodeunid} needed on add_node"
            ;;
        H)
            [ -z $debug ] || log_ "TODO: print HOME_ID: from device: https://github.com/Z-Wave-Alliance/z-wave-stack/issues/732"
            while [ ! -e "${mqtt_log}" ] ; do sleep 1; done
            [ "$homeid" != "" ] \
                || homeid=$(sed -n -e 's|ucl/by-unid/zw-\(.*\)-\([0-9]*\)/.*|\1|gp' "$mqtt_log" | tail -n1)
            echo "HOME_ID: ${homeid}"
            ;;
        n)
            nodeid=$(grep 'NODE_ID: ' $node_log \
                         | tail -n 1 | sed -e 's|NODE_ID: \(.*\)|\1|g' )
            echo "NODE_ID: ${nodeid}"
            nodeid=$(printf "%04d" $nodeid)
            nodeunid="zw-$homeid-$nodeid"
            echo "NODE_UNID: ${nodeunid}"
            ;;
        *)
            echo "TODO: handle $1"
            ;;
    esac
}


zpc_()
{
    controller_cli_ p
    file -E "$PTY"
    export ZPC_DEVICE=$(realpath -- "$PTY")
    if [[ "${ZPC_COMMAND}" = "/usr/bin/zpc" \
              || "${ZPC_COMMAND}" =~ .*build/.*/zpc.* ]] ; then
        ZPC_COMMAND="${ZPC_COMMAND} --zpc.serial=${ZPC_DEVICE}"
    fi
    ${ZPC_COMMAND}
    sleep 1
    zpc_cli_ version
    zpc_cli_ help
    die_
}


zpc_cli_()
{
    log_ "TODO: Fix console that eat some chars, and discard next workaround"
    log_ "TODO: https://github.com/SiliconLabsSoftware/z-wave-engine-application-layer/issues/30"
    if ! true ; then
        screen -S "$name" -p 3 -t zpc -X stuff "$@^M"
    else
        string="$@"
        for (( i=0; i<${#string}; i++ )); do
            char="${string:$i:1}"
            screen -S "$name" -p 3 -t zpc -X stuff "$char"
            sleep .1
        done
        screen -S "$name" -p 3 -t zpc -X stuff "^M"
        sleep 1
        screen -S "$name" -p 3 -t zpc -X hardcopy zpc_cli.log.tmp
    fi
}


play_uic_net_add_node_()
{
    log_ "net: controller: Add node (set in learn mode)"
    controller_cli_ H | grep "^HOME_ID: ........$" || die_
    controller_cli_ n | grep "^NODE_ID: " || die_

    sub="ucl/by-mqtt-client/zpc/ApplicationMonitoring/SupportedCommands"
    pub="$sub" # Can be anything
    message="{}"
    pubsub_ "$pub" "$message" "$sub"

    log_ "Find controller"
    sub="ucl/by-unid/+/ProtocolController/NetworkManagement"
    message="{}"
    expect='{"State":"idle","SupportedStateList":["add node","remove node","reset"]}'
    pubsub_ "$pub" "$message" "$sub" "$expect"

    log_ "net: node: Set to learn mode"
    homeid=$(sed -n -e 's|ucl/by-unid/zw-\(.*\)-\([0-9]*\)/.*|\1|gp' "$mqtt_sub_log")
    contid=$(sed -n -e 's|ucl/by-unid/zw-\(.*\)-\([0-9]*\)/.*|\2|gp' "$mqtt_sub_log")
    contunid="zw-$homeid-$contid"
    node_cli_ n | grep 'NODE_ID: 0' || die_
    node_cli_ l

    log_ "net: cont: Add node"
    pub="ucl/by-unid/$contunid/ProtocolController/NetworkManagement/Write"
    message='{"State":"add node"}'
    sub="ucl/by-unid/+/State/SupportedCommands"
    count="2" # NODEID=0001 is controller , NODEID=0002 is expected node
    expect="State/SupportedCommands"
    pubsub_ "$pub" "$message" "$sub" "$expect"
    node_cli_ H | grep "HOME_ID: ${homeid}" || die_
    node_cli_ n # Should not be 0
    pub=''
    sub=$(echo "$sub" | sed -e "s|/+/|/$nodeunid/|g")
    pubsub_ "$pub" "$message" "$sub" "$sub"

    node_cli_ d
    node_cli_ n

    log_ "Takes time from interviewing to functional"
    pub="ucl/by-unid/$contunid/ProtocolController/NetworkManagement/Write"
    message='{"State":"add node","StateParameters":{"UserAccept":true,"SecurityCode":"'${SecurityCode}'","AllowMultipleInclusions":false}}'
    sub="ucl/by-unid/$nodeunid/State"
    expect="$sub "'{"MaximumCommandDelay":1,"NetworkList":[""],"NetworkStatus":"Online functional","Security":"Z-Wave S2 Authenticated"}'
    count="3" # "NetworkStatus": "Online interviewing" *2
    pubsub_ "$pub" "$message" "$sub" "$expect" "$count"

    pub=""
    message=""
    sub="ucl/by-unid/+/State/Attributes/EndpointIdList/Reported"
    expect=$(echo "$sub "'{"value":[0]}' | sed -e "s|/+/|/$nodeunid/|g")
    pubsub_ "$pub" "$message" "$sub" "$expect" 2
    node_cli_ n # 2 expected on 1st time
}


play_uic_net_remove_node_()
{
    echo
    log_ "net: Remove node $nodeunid (~T738436)"
    controller_cli_ n > /dev/null

    pub="ucl/by-unid/$contunid/ProtocolController/NetworkManagement/Write"
    message='{"State":"remove node"}'
    sub="ucl/by-unid/+/State/SupportedCommands"
    node_cli_ n > /dev/null
    expect=$(echo "$sub (null)" | sed -e "s|/+/|/$nodeunid/|g")
    node_cli_ l
    pubsub_ "$pub" "$message" "$sub" "$expect" 3
    node_cli_ n | grep '^NODE_ID: 0$' || die_
}


play_uic_node_OnOff_()
{
    echo
    type="OnOff"
    node_cli_ n
    log_ "$type: Play on $nodeunid ~T738437 ~T738442"
    attribute="$type"

    message="{}"
    sub="ucl/by-unid/$nodeunid/ep0/$type/Attributes/$attribute/Reported"
    expect="$sub "'{"value":false}'
    sub_ "$sub" "$expect"
    sleep 1

    command="ForceReadAttributes"
    message="{ \"value\": [\"OnOff\"] }"
    pub="ucl/by-unid/$nodeunid/ep0/$type/Commands/$command"
    sub="ucl/by-unid/$nodeunid/ep0/$type/Attributes/$attribute/Reported"
    expect="$sub "'{"value":false}'
    pubsub_ "$pub" "$message" "$sub" "$expect"
    sleep 1

    command="Toggle" # T738442
    message="{}"
    pub="ucl/by-unid/$nodeunid/ep0/$type/Commands/$command"
    sub="ucl/by-unid/$nodeunid/ep0/$type/Attributes/$attribute/Reported"
    expect="$sub "'{"value":true}'
    pubsub_ "$pub" "$message" "$sub" "$expect"
    sleep 1

    expect="$sub "'{"value":false}'
    pubsub_ "$pub" "$message" "$sub" "$expect"
    sleep 1

    command="On" # T738437
    pub="ucl/by-unid/$nodeunid/ep0/$type/Commands/$command"
    expect="$sub "'{"value":true}'
    pubsub_ "$pub" "$message" "$sub" "$expect"
    sleep 1

    command="Off" # T738437
    pub="ucl/by-unid/$nodeunid/ep0/$type/Commands/$command"
    expect="$sub "'{"value":false}'
    pubsub_ "$pub" "$message" "$sub" "$expect"
    sleep 1

    log_ "$type: Events from device $nodeunid"
    node_cli_ 1 # From Off to On
    expect="$sub "'{"value":true}'
    sub_ "$sub" "$expect"
    node_cli_ 1 # From On to Off
    expect="$sub "'{"value":false}'
    sub_ "$sub" "$expect"
}


play_uic_()
{
    play_uic_net_add_node_
    play_uic_net_remove_node_

    play_uic_net_add_node_
    play_uic_node_OnOff_
    play_uic_net_remove_node_
}


play_()
{
    log_ "play: Wait for zpc mqtt ready"
    while ! grep -- "mqtt_wrapper_mosquitto" "${zpc_log}" ; do sleep 1 ; done
    controller_cli_ h
    node_cli_ h

    play_${zpc_api}_ || code=$?
    exit_ 0$code
}


setup_()
{
    local project="z-wave-stack-binaries"
    local url="https://github.com/Z-Wave-Alliance/${project}"
    local pattern="$project-*.tar.gz" # TODO: Bump latest release
    pattern="$project-25.1.0-25-g3b1e09d-Linux.tar.gz"

    mkdir -p "${project}" && cd "${project}"
    file -E "${pattern}" \
        || gh release download -R "$url" --pattern "$pattern"
    tar xvfz "${project}"*.tar.gz
}


default_()
{
    usage_
    sleep 1

    log_ "z-wave-stack-binaries: Check presence in ${z_wave_stack_binaries_bin_dir}"
    file -E "${z_wave_stack_binaries_bin_dir}/"*"REALTIME"*".elf"
    sleep 2

    cat <<EOF | tee "$file"
# https://www.gnu.org/software/screen/manual/screen.html#Command-Summary

hardstatus alwayslastline

split -v
screen -t "controller" 1 $0 run_ controller
sleep 1

split
focus down
screen -t "node" 2 $0 run_ node
sleep 1

split
focus down
screen -t "zpc" 3 $0 run_ zpc
sleep 2

focus right
screen -t "mqtt" 4 $0 run_ mqtt
sleep 1

split
focus down
screen -t "play (quit with: Ctrl+a \) " 5 $0 run_ play

EOF

    screen -wipe ||:
    screen -ls ||:

    # echo "disable tty to test detached mode"
    # exec </dev/null &>/dev/null
    local detached_opt=""
    [ -t 1 ] || detached_opt="-dm -L -Logfile /dev/stdout"
    screen $detached_opt -S "$name" -c "${file}"
    sleep 1

    local ref=$(date -u +%s)
    local delay=$((60 * ${duration}))
    local beat=10
    local expired=$(($delay + $ref))
    local now=$ref
    echo "info: Start watchdog to allow $duration minutes"
    while [ $now -le $expired ]; do
        screen -ls "$name" || break
        [ -z $debug ] || { ls -l *.log.tmp &&  more *.log.tmp | cat ; }
        tail "${mqtt_log}" ||:
        sleep $beat
        now=$(date -u +%s)
    done

    screen -S "$name" -X quit ||:
    cat "${mqtt_log}"

    code=$(cat ${code_log} || echo 254)
    exit 0$code
}


[ "" != "$1" ] || default_

"$@"

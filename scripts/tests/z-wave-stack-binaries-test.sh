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
CONFIG_PLAY_LOOP=${CONFIG_PLAY_LOOP:=false}
# TODO: https://github.com/Z-Wave-Alliance/z-wave-stack/pull/700
CONFIG_S2V2=${CONFIG_S2V2:=true}

# To be explicilty added to env
sudo="${sudo:=}"

duration=8 # Allocated time in mins until watchdog quit

ZPC_COMMAND="${ZPC_COMMAND:=/usr/bin/zpc}"

[ "" != "${z_wave_stack_binaries_bin_dir}" ] \
    || z_wave_stack_binaries_bin_dir="${PWD}/z-wave-stack-binaries/bin"

# Internal

name='z-wave'
code=0
log_suffix=".log.tmp"
code_log="code${log_suffix}"
zpc_log="zpc${log_suffix}"
file="screen.rc"
mqtt_pub_log="mqtt_pub${log_suffix}"
mqtt_sub_log="mqtt_sub${log_suffix}"
mqtt_log="mqtt${log_suffix}"
controller_log="ncp_serial_api_controller${log_suffix}"
node_log="node${log_suffix}"

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
    [ "$debug" != "" ] && echo \
            || clear >/dev/null 2>&1 || reset >/dev/null 2>&1 || :
    printf "${nocol}${yellow}info: $@ ${nocol}\n"
}


exit_()
{
    local code=0$1
    log_ "exit: $@ code=$code ; Use: debug=1 $0 # To trace script)"
    sleep 10
    [ -z "$debug" ] || sleep 1000
    echo "$code" | tee "$code_log"
    screen -S "$name" -X "quit" ||:
    ls -l -- *${log_suffix} && more -- *${log_suffix} | cat
    exit "0$code"
}


run_()
{
    local task="$1" && shift
    local log="$task${log_suffix}"
    rm -f "$log"
    "${task}_" "$@" 2>&1 | tee "$log"
}


mqtt_()
{
    local task="mqtt"
    log_ "$task: Wait for broker then flush queue before use and log traffic"
    while true ; do
        pidof mosquitto \
            && mosquitto_sub \
                   -v -t '#' --remove-retained --retained-only -W 1 \
                || [ 27 = $? ] && break ||: # Break on timeout
        sleep .1
    done
    log_ "$task: broker is ready, operating for ${duration} mins"
    local args=""
    [ $CONFIG_PLAY_LOOP ] || args="$args -W $((60 * ${duration}))"
    mosquitto_sub -v -t '#' $args
    log_ "$task: error: Should have finish before ${duration} may need to update it"
    exit_ 10
}


sub_()
{
    local sub="#"
    local count=1
    local expect=""
    local delay=0

    [ "$1" = "" ] || sub="$1"
    [ "$2" = "" ] || expect="$2"
    [ "$3" = "" ] || count="$3"
    [ "$4" = "" ] || delay="$4"
    [ "" != "$sub" ] || sub="$pub"
    [ "" != "$expect" ] || expect="$sub"

    printf "${nocol}${cyan}sub: $sub${nocol} (count=${count})\n"
    mosquitto_sub -v -t "$sub" -C $count | tee "$mqtt_sub_log"
    [ "" = "$expect" ] || printf "${nocol}${blue}exp: $expect${nocol}\n"

    if [ "" != "$expect" ] ; then
        tail -n +$count "$mqtt_sub_log" \
            | head -n1 | grep -E "$expect" 2>&1 > /dev/null \
            || { printf "${nocol}${red}exp: error:: ${expect}${nocol}\n" ;
                 cat "$mqtt_sub_log"  ;
                 exit_ 11; }
    fi
    sleep $delay
}


pub_()
{
    local topic="$1"
    local message="$2"
    log_ "pub: $@"

    [ "" = "$message" ] || printf "$message" | jq
    sleep 1
    mosquitto_pub -t "$topic" -m "$message"
    sleep 1
}


pubsub_()
{
    echo ""
    [ "" = "$debug" ] || log_ "pubsub_: $@"
    local pub="$1"
    local message="$2"
    local sub=""
    local expect=""
    local delay=1
    local count=1

    [ "$3" = "" ] || sub="$3"
    [ "$4" = "" ] || expect="$4"
    [ "$5" = "" ] || count="$5"
    [ "$6" = "" ] || delay="$6"
    [ "" != "$sub" ] || sub="$pub"
    [ "" != "$expect" ] || expect="$sub"

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
    else
        sleep 1
    fi
    [  0 -eq 0$code ] || exit_ $code # Quit on 1st failure
}


### Applications

app_()
{
    local app="$1"
    [ "" != "$app" ] || app="soc_switch_on_off"
    local app_mode="${app_mode:=REALTIME}"
    local app_suffix="_x86_${app_mode}.elf"
    local app_file=$(realpath \
                         "${z_wave_stack_binaries_bin_dir}/ZW_zwave_${app}"*"${app_suffix}" \
                         | head -n1)
    [ "$debug" = "" ] || file -E "${app_file}"
    "${app_file}" --pty
}


run_app_()
{
    local app="$1" && shift
    local log="$app${log_suffix}"
    rm -f "$log"
    app_ "$app" "$@" 2>&1 | tee "$log"
}


controller_cli_()
{
    local app="ncp_serial_api_controller"
    until [ -e "${controller_log}" ] ; do sleep 1; done
    sleep 1
    screen -S "$name" -p "${app}" -t "controller: $@" -X stuff "$@^M"
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
            homeid=$(grep 'HOME_ID: ' "${controller_log}" \
                         | tail -n 1 | sed -e 's|HOME_ID: \(.*\)|\1|g' )
            echo "HOME_ID: ${homeid}"
            [ ! -z $homeid ] || exit_ 19
            conthomeid="$homeid"
            ;;
        n)
            contid=$(grep 'NODE_ID: ' "${controller_log}" \
                         | tail -n 1 | sed -e 's|NODE_ID: \(.*\)|\1|g' )
            echo "NODE_ID: ${contid}"
            contidhex=$(printf "%04X" $contid)
            contunid="zw-$homeid-$contidhex" # Unify Node Id
            echo "NODE_UNID: ${contunid}"
            ;;
        *)
            echo "TODO: handle $1"
            ;;
    esac
}


node_cli_()
{
    local node
    [ -z $1 ] || { node="$1" && shift ; }
    [ ! -z $node ] || node="soc_switch_on_off"
    local node_log="$node${log_suffix}"
    until [ -e "${node_log}" ] ; do sleep 1; done
    screen -S "$name" -p "${node}" -X stuff "$@^M"
    sleep 1
    case $1 in
        h)
            echo "node: ${node}: Should display help"
            ;;
        d)
            local DSK=$(grep 'DSK: ' "${node_log}" \
                      | tail -n 1 | sed -e 's|DSK: \([0-9-]*\)|\1|g' )
            SecurityCode=$(echo "$DSK" | sed -e 's|\([0-9]*\)-[0-9-]*$|\1|g')
            ;;
        l)
            echo "node: ${node}: Set to learn mode ${nodeid} needed on add_node"
            ;;
        H)
            nodehomeid=$(grep 'HOME_ID: ' "${node_log}" \
                             | tail -n 1 | sed -e 's|HOME_ID: \(.*\)|\1|g' )
            echo "HOME_ID: ${nodehomeid}"
            [ ! -z $nodehomeid ] || exit_ 19
            ;;
        n)
            nodeid=$(grep 'NODE_ID: ' $node_log \
                         | tail -n 1 | sed -e 's|NODE_ID: \(.*\)|\1|g' )
            echo "NODE_ID: ${nodeid}"
            nodeidhex=$(printf "%04X" $nodeid)
            nodeunid="zw-$nodehomeid-$nodeidhex" # Unify Node Id
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
    exit_ 13
}


zpc_cli_()
{
    log_ "TODO: Fix console that eat some chars, and discard next workaround"
    log_ "TODO: https://github.com/SiliconLabsSoftware/z-wave-engine-application-layer/issues/30"
    if ! true ; then
        screen -S "$name" -p "zpc" -t zpc -X stuff "$@^M"
    else
        string="$@"
        for (( i=0; i<${#string}; i++ )); do
            char="${string:$i:1}"
            screen -S "$name" -p "zpc" -t zpc -X stuff "$char"
            sleep .01
        done
        screen -S "$name" -p "zpc" -t zpc -X stuff "^M"
        sleep 1
        screen -S "$name" -p "zpc" -t zpc -X hardcopy zpc_cli${log_suffix}
    fi
}


play_net_add_node_()
{
    local node="soc_switch_on_off"
    [ -z "$1" ] || node="$1"

    local command="add_node"
    log_ "net: $command: Node should not be included: $nodeid ($node)"
    node_cli_ "$node" H
    node_cli_ "$node" n
    [ 0 -eq 0$nodeid ] || exit_ 21

    log_ "net: Search for controller"
    controller_cli_ H
    [ ! -z "$conthomeid" ] || exit_ 25
    controller_cli_ n
    [ ! -z "$contid" ] || exit_ 26

    log_ "net: $command: Use controller: $contid"
    sub="ucl/by-unid/$contunid/State"
    json='{"MaximumCommandDelay":0,"NetworkList":[""],"NetworkStatus":"Online functional","Security":"Z-Wave S2 Access Control"}'
    expect=$(echo "$json" | sed 's/\[/\\[/g; s/\]/\\]/g')
    sub_ "$sub" "$expect"

    log_ "net: ${command}: node: Set to learn mode: $nodeid: $node"
    node_cli_ "$node" n
    [ 0 -eq 0$nodeid ] || exit_ 16
    node_cli_ "$node" l

    log_ "net: ${command}: inclusion: $nodeid into ${homeid}"
    pub="ucl/by-unid/$contunid/ProtocolController/NetworkManagement/Write"
    message='{"State":"add node"}'
    sub="ucl/by-unid/+/State/SupportedCommands"
    count="2" # NODEID=0001 is controller , NODEID=0002 is expected node
    expect="State/SupportedCommands"
    pubsub_ "$pub" "$message" "$sub" "$expect"
    node_cli_ "$node" H
    # TODO: Issue observed after ~24h on sensor:
    [ $conthomeid = $nodehomeid ] || exit_ 17
    node_cli_ "$node" n # Should not be 0
    pub=''
    sub=$(echo "$sub" | sed -e "s|/+/|/$nodeunid/|g")
    pubsub_ "$pub" "$message" "$sub"

    node_cli_ "$node" d
    node_cli_ "$node" n

    log_ "net: ${command}: Pass SecurityCode=${SecurityCode} of $nodeid to controller ($node)"
    pub="ucl/by-unid/$contunid/ProtocolController/NetworkManagement/Write"
    message='{"State":"add node","StateParameters":{"UserAccept":true,"SecurityCode":"'${SecurityCode}'","AllowMultipleInclusions":false}}'
    sub="ucl/by-unid/$nodeunid/State"
    pubsub_ "$pub" "$message" "$sub"

    NetworkStatus='.*' # Match: [ "Online interviewing", "Online functional" ]
    MaximumCommandDelay='.*' # Variable: 1, 300
    Security='.*'
    json='{"MaximumCommandDelay":'${MaximumCommandDelay}',"NetworkList":[""],"NetworkStatus":"'${NetworkStatus}'","Security":"'${Security}'"}'
    expect=$(echo "$json" | sed 's/\[/\\[/g; s/\]/\\]/g')
    expect="$sub $expect"

    NetworkStatus='Online functional'
    Security="Z-Wave S2 .*" # Authenticated or AccessControl (Doorlock)
    json='{"MaximumCommandDelay":'${MaximumCommandDelay}',"NetworkList":[""],"NetworkStatus":"'${NetworkStatus}'","Security":"'${Security}'"}'
    over_expect=$(echo "$json" | sed 's/\[/\\[/g; s/\]/\\]/g')
    over_expect="$sub $over_expect"
    local over=false
    while ! $over ; do # Multiple steps: "Online interviewing"+
        sub_ "$sub" "$expect"
        grep -E "$over_expect" "$mqtt_sub_log" && over=true || sleep 5
    done

    node_cli_ "$node" H # expected on 1st time
    [ $conthomeid = $nodehomeid ] || exit_ 17
    node_cli_ "$node" n # 2 expected on 1st time
    [ $nodeid -ne 0 ] || exit_ 19

    zpc_cli_ zwave_log_security_keys
}


play_net_remove_node_()
{
    local node="soc_switch_on_off"
    [ -z $1 ] || node="$1"

    [ 0 -ne 0$nodeid ] || exit_ 19

    echo
    command="remove_node"
    log_ "net: $command: $nodeid ($node) " # ~T738436
    controller_cli_ n > /dev/null

    pub="ucl/by-unid/$contunid/ProtocolController/NetworkManagement/Write"
    message='{"State":"remove node"}'
    sub="ucl/by-unid/+/State/SupportedCommands"
    node_cli_ "$node" n > /dev/null
    expect='(null)'
    expect=$(echo "$expect" | sed -e 's|[()]|\\&|g')
    expect=$(echo "$sub $expect" | sed -e "s|/+/|/$nodeunid/|g")
    node_cli_ "$node" l
    pubsub_ "$pub" "$message" "$sub" "$expect" 3 # TODO
    node_cli_ "$node" n
    [ 0 -eq $nodeid ] || exit_ 19
}


play_node_soc_switch_on_off_()
{
    echo
    local node="soc_switch_on_off"
    local type="OnOff"

    node_cli_ "$node" n
    log_ "$node: Play on $nodeid " # ~T738437 ~T738442"
    local attribute="$type"

    log_ "$node: Initial state reported after inclusion ($attribute)"
    message="{}"
    sub="ucl/by-unid/$nodeunid/ep0/$type/Attributes/$attribute/Reported"
    json='{"value":false}'
    expect="$sub $json"
    sub_ "$sub" "$expect"

    command="ForceReadAttributes"
    message="{ \"value\": [\"OnOff\"] }"
    pub="ucl/by-unid/$nodeunid/ep0/$type/Commands/$command"
    sub="ucl/by-unid/$nodeunid/ep0/$type/Attributes/$attribute/Reported"
    pubsub_ "$pub" "$message" "$sub" "$expect"

    command="Toggle" # T738442
    message="{}"
    pub="ucl/by-unid/$nodeunid/ep0/$type/Commands/$command"
    sub="ucl/by-unid/$nodeunid/ep0/$type/Attributes/$attribute/Reported"
    json='{"value":true}'
    expect="$sub $json"
    pubsub_ "$pub" "$message" "$sub" "$expect"

    json='{"value":false}'
    expect="$sub $json"
    pubsub_ "$pub" "$message" "$sub" "$expect"

    command="On" # T738437
    pub="ucl/by-unid/$nodeunid/ep0/$type/Commands/$command"
    json='{"value":true}'
    expect="$sub $json"
    pubsub_ "$pub" "$message" "$sub" "$expect"

    command="Off" # T738437
    pub="ucl/by-unid/$nodeunid/ep0/$type/Commands/$command"
    json='{"value":false}'
    expect="$sub $json"
    pubsub_ "$pub" "$message" "$sub" "$expect"

    log_ "$node: Events from device $nodeid"
    node_cli_ "$node" 1 # From Off to On
    json='{"value":true}'
    expect="$sub $json"
    sub_ "$sub" "$expect"
    node_cli_ "$node" 1 # From On to Off
    json='{"value":false}'
    expect="$sub $json"
    sub_ "$sub" "$expect"
}


play_node_soc_multilevel_sensor_()
{
    local node="soc_multilevel_sensor"
    node_cli_ "$node" n
    log_ "$node: Play on $nodeid" # ~T738437 ~T738442

    log_ "$node: Initial state reported after inclusion"

    local ep="ep0/Basic/Attributes/PowerSource/Reported"
    local sub="ucl/by-unid/${nodeunid}/$ep"
    local key="value" ; key='"'$key'"' # JSON string
    local value='Battery' ; value='"'$value'"'
    local json='{'$key':'$value'}'
    local expect="$sub $json"
    sub_ "$sub" "$expect"

    ep='ep0/TemperatureMeasurement/Attributes/MeasuredValue/Reported'
    sub="ucl/by-unid/${nodeunid}/$ep"
    value=322
    json='{'$key':'$value'}'
    expect="$sub $json"
    sub_ "$sub" "$expect"

    ep='ep0/RelativityHumidity/Attributes/MeasuredValue/Reported'
    sub="ucl/by-unid/${nodeunid}/$ep"
    value=8
    json='{'$key':'$value'}'
    expect="$sub $json"
    sub_ "$sub" "$expect"

    ep="ep0/PowerConfiguration/Attributes/BatteryPercentageRemaining/Reported"
    sub="ucl/by-unid/${nodeunid}/$ep"
    value=100
    json='{'$key':'$value'}'
    expect="$sub $json"
    sub_ "$sub" "$expect"

    log_ "$type: Events from device $nodeunid: $node"
    node_cli_ "$node" 1
    sub_ "$sub" "$expect"
}


play_node_soc_door_lock_keypad_()
{
    local node="soc_door_lock_keypad"
    local type="DoorLock"
    local attribute="LockState"

    node_cli_ "$node" n
    log_ "$node: Play on $nodeid" # ~T738437 ~T738442

    log_ "$node: Initial state reported after inclusion ($attribute)"

    local ep="ep0/$type/SupportedCommands"
    local sub="ucl/by-unid/${nodeunid}/$ep"
    local key="value" ; key='"'$key'"' # JSON string
    local value='["LockDoor", "UnlockDoor", "Toggle", "SetPINCode", "GetPINCode", "ClearPINCode", "ClearAllPINCodes", "SetUserStatus", "GetUserStatus", "ForceReadAttributes"]';
    local json='{'$key': '$value'}'
    json=$(echo "$json" | sed 's/\[/\\[/g; s/\]/\\]/g')
    local expect="$sub $json"
    sub_ "$sub" "$expect"

    sub="ucl/by-unid/$nodeunid/ep0/$type/Attributes/$attribute/Reported"
    value='Unlocked'
    json='{"value":"'$value'"}'
    expect="$sub $json"
    sub_ "$sub" "$expect"

    local command="Toggle"
    local code="0000" # Any code is mandatory to prevent json parsing error (TODO)
    local message='{"PINOrRFIDCode":"'$code'"}'
    local pub="ucl/by-unid/$nodeunid/ep0/$type/Commands/$command"
    sub="ucl/by-unid/$nodeunid/ep0/$type/Attributes/$attribute/Reported"
    value='Locked'
    json='{"value":"'$value'"}'
    expect="$sub $json"
    pubsub_ "$pub" "$message" "$sub" "$expect"

    type="UserCredential"
    command="AddUser"
    ep="ep0/$type/Commands/$command"
    pub="ucl/by-unid/$nodeunid/$ep"
    local userid=1
    local usertype='ProgrammingUser'

    message='
{
      "UserUniqueID": '$userid',
      "UserType": "'$usertype'",
      "UserActiveState": false,
      "CredentialRule": "Single",
      "UserName": "Admin",
      "ExpiringTimeoutMinutes": 0,
      "UserNameEncoding": "ASCII"
}
'
    sub="ucl/by-unid/$nodeunid/ep0/$type/Attributes/User/$userid/UserName/Reported"
    expect='{"value":"Admin"}'
    expect="$sub $expect"
    pubsub_ "$pub" "$message" "$sub" "$expect"

    userid=2
    usertype='GeneralUser'
    message='
{
      "UserUniqueID": '$userid',
      "UserType": "'$usertype'",
      "UserActiveState": false,
      "CredentialRule": "Single",
      "UserName": "",
      "ExpiringTimeoutMinutes": 0,
      "UserNameEncoding": "ASCII"
}
'
    sub="ucl/by-unid/$nodeunid/ep0/$type/Attributes/User/$userid/UserName/Reported"
    expect='{"value":"User-'$userid'"}'
    expect="$sub $expect"
    pubsub_ "$pub" "$message" "$sub" "$expect"

    command='AddCredential'
    ep="ep0/$type/Commands/$command"
    pub="ucl/by-unid/$nodeunid/$ep"
    value=1234
    message='{
      "UserUniqueID": '$userid',
      "CredentialType": "PINCode",
      "CredentialSlot": 5,
      "CredentialData": "'$value'"
}'
    ep="ep0/UserCredential/Attributes/User/2/Credential/PINCode/5/CredentialData/Reported"
    sub="ucl/by-unid/$nodeunid/$ep"
    expect='{"value":"'$value'"}'
    expect="$sub $expect"
    pubsub_ "$pub" "$message" "$sub" "$expect"

    command='ModifyCredential'
    ep="ep0/$type/Commands/$command"
    pub="ucl/by-unid/$nodeunid/$ep"
    value=5678
    message='{
      "UserUniqueID": '$userid',
      "CredentialType": "PINCode",
      "CredentialSlot": 5,
      "CredentialData": "'$value'"
}'
    expect='{"value":"'$value'"}'
    expect="$sub $expect"
    pubsub_ "$pub" "$message" "$sub"  "$expect"

    command='DeleteAllCredentials'
    ep="ep0/$type/Commands/$command"
    pub="ucl/by-unid/$nodeunid/$ep"
    message='{}'
    sub="ucl/by-unid/$nodeunid/ep0/$type/Attributes/User/+/Credential/PINCode/+/CredentialModifierNodeId/Reported"
    expect="$sub (null)"
    pub_ "$pub" "$message" # Not retained ?

    command="DeleteAllUsers"
    ep="ep0/$type/Commands/$command"
    pub="ucl/by-unid/$nodeunid/$ep"
    message='{}'
    sub="ucl/by-unid/$nodeunid/ep0/UserCredential/Attributes/User/1/UserName/Reported"
    pub_ "$pub" "$message" # Not retained ?
}


play_node_s2v2_()
{
    local task="s2v2"
    # log_ "$task: TODO: https://github.com/Z-Wave-Alliance/z-wave-stack/pull/700"
    local type="OnOff"
    node_cli_ "$node" H
    node_cli_ "$node" n
    echo "info: Play $type on $nodeunid"

    local command="EnableNls"
    local pub="ucl/by-unid/$nodeunid/State/Commands/$command"
    local message="{}"
    log_ "TODO: Expect response in MQTT, workaround by looking at debug log"
    log_ "TODO: https://github.com/SiliconLabsSoftware/z-wave-engine-application-layer/issues/31"
    pub_ "$pub" "$message" "" # TODO use pub/sub MQTT not shell (next line)
    # zpc_cli_ zwave_enable_nls ${nodeid}
    sleep 1
    grep 'on_nls_state_set_v2_send_complete' "${zpc_log}" || exit_ 20
    grep 'on_nls_state_get_v2_send_complete' "${zpc_log}" || exit_ 21
    zpc_cli_ "attribute_store_log_search" "NLS"
    
    zpc_cli_ "attribute_store_log_search" "NLS state"
    grep -2 -E ".* NodeID [.]*[ ]* ${nodeid}" "${zpc_log}" \
        | grep -E 'NLS state [.]*' \
        || exit_ 22 # 2 expected
    
    zpc_cli_ "attribute_store_log_search" "NLS support"
    grep -3 -E ".* NodeID [.]*[ ]* ${nodeid}" "${zpc_log}" \
        | grep -E 'NLS support [.]*.*1.*' \
        || exit_ 23
    pub="ucl/by-unid/$nodeunid/State/Commands/DiscoverNeighbors"
    message='{}'
    pub_ "$pub" "$message"
    grep 'ucl_nm_neighbor_discovery' "${zpc_log}" || exit_ 24
}

play_node_()
{
    local node="$1"
    if true ; then
        play_net_add_node_ $node
        play_net_remove_node_ $node
    fi

    if true ; then
        play_net_add_node_ $node
        play_node_${node}_
        play_net_remove_node_ $node
    fi
    if ${CONFIG_S2V2} ; then
        play_net_add_node_ $node
        play_node_s2v2_
        play_net_remove_node_ $node
    else
        log_ "TODO: https://github.com/Z-Wave-Alliance/z-wave-stack/pull/700"
    fi
}


play_nodes_()
{
    local nodes=(
        soc_switch_on_off
        soc_multilevel_sensor
        soc_door_lock_keypad
    )
    for node in ${nodes[@]} ; do
        node_cli_ $node h
        play_node_ $node || code=$?
        [ 0$code -eq 0 ] || break
    done
}


play_()
{
    local task="play"
    log_ "$task: Wait for zpc mqtt ready"
    until grep -- "\[mqtt_wrapper_mosquitto\]" "${zpc_log}" ; do sleep 1 ; done
    until grep -- "\[mqtt_client\] Connection to MQTT broker" "${zpc_log}" ; do sleep 1 ; done

    log_ "$task: Check presence of controller"
    controller_cli_ h

    log_ "$task: Find host"
    sub="ucl/by-mqtt-client/zpc/ApplicationMonitoring/SupportedCommands"
    sub_ "$sub"

    log_ "$task: Find controller API"
    sub="ucl/by-unid/+/ProtocolController/NetworkManagement"
    json='{"State":"idle","SupportedStateList":["add node","remove node","reset"]}'
    expect=$(echo "$json" | sed 's/\[/\\[/g; s/\]/\\]/g')
    sub_ "$sub" "$expect"

    local code=0
    local over=false
    until $over ; do
        ${CONFIG_PLAY_LOOP} || over=true
        play_nodes_ || code=$?
        [ 0 -eq 0$code ] || break
        echo over=$over
    done

    log_ "$task: exit: $code"
    exit_ 0$code
}


setup_()
{
    local project="z-wave-stack-binaries"
    local url="https://github.com/Z-Wave-Alliance/${project}"
    local pattern="$project-*.tar.gz" # TODO: Bump latest release
    local rev="25.1.0-26-g29d304"
    pattern="$project-${rev}-Linux.tar.gz"

    mkdir -p "${project}" && cd "${project}"
    file -E "${pattern}" \
        || gh release download -R "$url" --pattern "$pattern"
    tar xvfz "${project}"*.tar.gz
    # TODO: https://github.com/eclipse-mosquitto/mosquitto/issues/3267
    mosquitto_pub --version \
        || which mosquitto_pub \
        || $sudo apt install mosquitto-clients
}


default_()
{
    usage_
    sleep 1

    log_ "Setup check, if failing please setup using:"
    echo "sudo=sudo $0 setup_"

    [ "" = "$debug" ] || set

    screen --version

    # TODO: https://github.com/eclipse-mosquitto/mosquitto/issues/3267
    mosquitto_pub --version \
        || which mosquitto_pub

    log_ "z-wave-stack-binaries: Check presence in ${z_wave_stack_binaries_bin_dir}"
    file -E "${z_wave_stack_binaries_bin_dir}/"*"REALTIME"*".elf"
    sleep 1

    cat <<EOF | tee "$file"
# https://www.gnu.org/software/screen/manual/screen.html#Command-Summary

hardstatus alwayslastline
split -v
focus left

split
focus up
screen -t "ncp_serial_api_controller" "1" $0 run_app_ ncp_serial_api_controller

split -v
focus right
screen -t "soc_switch_on_off" "2" $0 run_app_ soc_switch_on_off

split -v
focus right
screen -t "soc_multilevel_sensor" "3" $0 run_app_ soc_multilevel_sensor

split -v
focus right
screen -t "soc_door_lock_keypad" "4" $0 run_app_ soc_door_lock_keypad

focus down
screen -t "zpc" "0" $0 run_ zpc

focus right
screen -t "mqtt" "8" $0 run_ mqtt

split
focus down
screen -t "play (quit with: Ctrl+a \)" "9" $0 run_ play

EOF

    screen -wipe ||:
    screen -ls ||:

    # echo "disable tty to test detached mode"
    # exec </dev/null &>/dev/null
    local detached_opt=""
    [ -t 1 ] || detached_opt="-dm -L -Logfile /dev/stdout"
    screen $detached_opt -S "$name" -c "${file}"
    sleep 1

    local delay=$((60 * ${duration}))
    if $CONFIG_PLAY_LOOP ; then
        echo "info: Will need to be interrupted manually: pid=$!"
        while true ; do sleep $delay ; done
    else
        local ref=$(date -u +%s)
        local beat=10
        local expired=$(($delay + $ref))
        local now=$ref
        echo "info: Start watchdog to allow $duration minutes"
        while [ $now -le $expired ]; do
            screen -ls "$name" || break
            [ -z $debug ] || { ls -l *${log_suffix} && more *${log_suffix} | cat ; }
            more "${mqtt_log}" | tail ||:
            sleep $beat
            now=$(date -u +%s)
        done
    fi
    screen -S "$name" -X quit ||:
    cat "${mqtt_log}"

    code=$(cat ${code_log} || echo 254)
    exit_ 0$code
}


[ "" != "$1" ] || default_

"$@"

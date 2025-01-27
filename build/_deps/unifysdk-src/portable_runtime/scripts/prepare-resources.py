###############################################################################
## License
## <b>Copyright 2022  Silicon Laboratories Inc. www.silabs.com</b>
###############################################################################
## The licensor of this software is Silicon Laboratories Inc. Your use of this
## software is governed by the terms of Silicon Labs Master Software License
## Agreement (MSLA) available at
## www.silabs.com/about-us/legal/master-software-license-agreement. This
## software is distributed to you in Source Code format and is governed by the
## sections of the MSLA applicable to Source Code.
##
###############################################################################

import os
import glob
import shutil
import argparse
from urllib.parse import urlparse
import tempfile
import hashlib
from pathlib import Path
import requests

platform_tools = {
    'linux' : {
        'target_folder'   : 'x86_64-unknown-linux-gnu/$build_type$/resources',
        'resources'       : [
            {
                'download_name'   : 'Commander',
                'download_url'    : 'https://www.silabs.com/documents/public/software/SimplicityCommander-Linux.zip',
                'download_sha256' : '4bc0a3f402bb8cedb2d081e0411d56bf4fe91f5439ff291deef0fa039f8d2bc9',
                'extract_details' : [
                    {
                        'extract_src' : 'SimplicityCommander-Linux/Commander_linux_x86_64_1v17p1b1824.tar.bz',
                        'extract_dst' : 'docker-files/',
                    }
                ]
            }
        ]
    },
    'macos' : {
        'target_folder'   : 'x86_64-apple-darwin/$build_type$/resources',
        'resources'       : [
            {
                'download_name'   : 'Commander',
                'download_url'    : 'https://www.silabs.com/documents/public/software/SimplicityCommander-Mac.zip',
                'download_sha256' : '53cd8caf3ae03ac11e030855afa4165c5e819276548e7b3cb53cfe7e8bcae254',
                'extract_details' : [
                    {
                        'extract_src' : 'SimplicityCommander-Mac/Commander_osx_1v17p1b1824.zip',
                        'extract_dst' : 'docker-files/',
                    }
                ]
            }
        ]
    },
    'windows' : {
        'target_folder'   : 'x86_64-pc-windows-gnu/$build_type$/resources',
        'resources'       : [
            {
                'download_name'   : 'Commander',
                'download_url'    : 'https://www.silabs.com/documents/public/software/SimplicityCommander-Windows.zip',
                'download_sha256' : '88002499c46d957f98a92a88483de6bf3675a34280143075b64e17e79e509a24',
                'extract_details' : [
                    {
                        'extract_src' : 'SimplicityCommander-Windows/Commander_win32_x64_1v17p1b1824.zip',
                        'extract_dst' : 'docker-files/',
                    }
                ]
            }
        ]
    }
}

common_resources = [
    {
        'download_name'   : 'Application Firmware',
        'download_url'    : 'https://github.com/SiliconLabs/simplicity_sdk/releases/download/v2024.6.0/demo-applications.zip',
        'download_sha256' : '0f8d7b614aaee962a7e7b9e34dd9d322bdd41fed5ffa658e5cb4e902bf4985fe',
        'extract_details' : [
            {
                'extract_src' : 'protocol/z-wave/demos/zwave_soc_switch_on_off/zwave_soc_switch_on_off-brd2603a-eu.hex',
                'extract_dst' : 'app-binaries/zwave_soc_switch_on_off-brd2603a-eu.hex',
            },
            {
                'extract_src' : 'protocol/z-wave/demos/zwave_soc_multilevel_sensor/zwave_soc_multilevel_sensor-brd2603a-eu.hex',
                'extract_dst' : 'app-binaries/zwave_soc_multilevel_sensor-brd2603a-eu.hex',
            },
            {
                'extract_src' : 'protocol/z-wave/demos/zwave_ncp_serial_api_controller/zwave_ncp_serial_api_controller-brd2603a-eu.hex',
                'extract_dst' : 'app-binaries/zwave_ncp_serial_api_controller-brd2603a-eu.hex',
            }
        ]
    },
    {
        'download_name'   : 'Unify Debian packages',
        'download_url'    : 'https://github.com/SiliconLabs/UnifySDK/releases/download/ver_1.6.0/unify_1.6.0_x86_64.zip',
        'download_sha256' : 'eb98c7f1e17e6680b6fb93b45aed6cab6ee336bfdc67128eb6c001390df342d6',
        'extract_details' : [
            {
                'extract_dst' : 'docker-files/',
            }
        ],
    },
    {
        'download_name'   : 'Matter Bridge Binaries',
        'download_url'    : 'https://github.com/SiliconLabs/unify-matter-bridge/releases/download/ver_1.3.0-1.3/unify-matter-bridge-1.3.0-1.3-amd64.zip',
        'download_sha256' : '302888dcc4bddc9e22e5bd4cc03ca25372ab60fc406716e719ee525ffba6b781',
        'extract_details' : [
             {
                'extract_src' : 'uic-mb_1.3.0_amd64/chip-tool',
                'extract_dst' : 'docker-files/',
            },
            {
                'extract_src' : 'uic-mb_1.3.0_amd64/uic-mb_1.3.0_amd64.deb',
                'extract_dst' : 'docker-files/',
            }
        ],
    },
    {
        'download_name'   : 'EED Debian',
        'download_url'    : '',
        'download_sha256' : '',
        'extract_details' : [
            {
                'extract_src' : 'eed/uic-eed_amd64.deb',
                'extract_dst' : 'docker-files/',
            }
        ],
    }
]

def check_commander_files(folder_path):
    # Define the patterns to search for
    patterns = ["Commander_*.zip", "Commander_*.tar"]
    
    # Check for files matching the patterns
    for pattern in patterns:
        if glob.glob(os.path.join(folder_path, pattern)):
            return True
    return False

def download_file(download_folder, link):
    file_name = link.split('/')[-1]
    # Removes query parameters, if present.
    file_name = file_name.split('?')[0]
    file_path = f"{download_folder}/{file_name}"
    r = requests.get(link, stream = True)
    with open(file_path, 'wb') as f:
        for chunk in r.iter_content(chunk_size = 1024*1024):
            if chunk:
                f.write(chunk)

    return file_path

def extract_file_to_folder(compressed_file, to_folder):
    print(f"Extracting {compressed_file} to {to_folder}")

    os.system(f"rm -rf {to_folder} && mkdir -p {to_folder}")

    if compressed_file.endswith('.zip'):
        status = os.system(f"unzip -q {compressed_file} -d {to_folder}")
        assert status == 0, f"Failed extracting {compressed_file}"
    else :
        status = os.system(f"tar xf {compressed_file} -C {to_folder}")
        assert status == 0, f"Failed extracting {compressed_file}"

        print(f"Extraction of {compressed_file} done!")

def package_binary(search_path, to_folder):
    binary_file = glob.glob(search_path)

    assert (
        len(binary_file) == 1
    ), f"Found {len(binary_file)} file(s) fitting the pattern: {search_path}"
    shutil.copy(binary_file[0], to_folder)

def zip_folder(directory: str):
    zip_file = directory + ".zip "
    try:
        print("Removing any existing Zip package")
        os.remove(zip_file)
        print(zip_file + " removed successfully")
    except OSError as error:
        pass

    print(f"Zipping {directory} as {directory}.zip")
    status = os.system("zip --symlinks -r " + directory + ".zip " + directory)
    assert status == 0, f"Failed zipping folder {directory}"
    shutil.rmtree(directory)

def verify_hash(file_path, known_hash):
    print(f"Verifying hash for file: {file_path}")
    print(f"Known hash:      {known_hash}")
    sha256_hash = hashlib.sha256()
    with open(file_path, "rb") as f:
        # Read and update hash string value in blocks of 4K
        for byte_block in iter(lambda: f.read(4096),b""):
            sha256_hash.update(byte_block)

        print(f"Calculated hash: {sha256_hash.hexdigest()}")
        assert (sha256_hash.hexdigest() == known_hash)

def get_local_resources(resource, target_folder, src_file_path):
    with tempfile.TemporaryDirectory() as extract_folder:
        extract_srcs = list(filter(lambda e: 'extract_src' in e, resource['extract_details']))

        if len(extract_srcs) > 0:
            extract_file_to_folder(src_file_path, extract_folder)

        for extract in resource['extract_details']:
            move_from = f"{extract_folder}/{extract['extract_src']}" if 'extract_src' in extract else src_file_path
            move_to = f"{target_folder}/{extract['extract_dst']}"

            move_to_path = Path(move_to)

            move_from_path = Path(move_from)
            print(f"Copying {move_from} to {move_to}")
            if os.path.isdir(move_from_path):
                shutil.rmtree(move_to_path, ignore_errors=True)
                shutil.copytree(move_from_path, move_to_path, symlinks=True)
            else:
                os.makedirs(move_to_path.parent.absolute(), exist_ok=True)
                shutil.copy(move_from_path, move_to_path)

def get_resources(resource, target_folder):
    with tempfile.TemporaryDirectory() as download_folder_path:
        print(f"Downloading {resource['download_name']} from {resource['download_url']}")
        file_path = download_file(download_folder_path, resource['download_url'])

        print(f"Downloaded file path: {file_path}")
        verify_hash(file_path, resource['download_sha256'])

        get_local_resources(resource, target_folder, file_path)

def arguments_parsing():
    parser = argparse.ArgumentParser(
        description=__doc__)
    parser.add_argument("--target", choices=["windows", "linux", "macos"], required=True)
    parser.add_argument("--build_folder", default=os.path.join(os.path.dirname(__file__), "../target"))
    parser.add_argument("--build_type", choices=["debug", "release"], default="release")
    parser.add_argument("--unify_path", type=str, required=False)
    parser.add_argument("--resource_path", type=str, default=os.path.join(os.path.dirname(__file__), "../resources_local"))
    parser.add_argument("--unify_matter_bridge_path", type=str, required=False)
    args = parser.parse_args()
    return args

if __name__ == "__main__":
    global args
    args = arguments_parsing()

    resource_folder = os.path.join(os.path.dirname(__file__), "../resources")
    platform_resources = platform_tools[args.target]

    copy_dst = os.path.join(args.build_folder, platform_resources['target_folder'])
    copy_dst = f"{copy_dst}".replace("$build_type$", args.build_type)

    shutil.rmtree(copy_dst, ignore_errors=True)
    shutil.copytree(resource_folder, copy_dst, symlinks=True)

    for resource in platform_resources['resources']: 
        if resource['download_name'] == 'Commander' and args.resource_path is not None:
            
            if check_commander_files(args.resource_path) :
                commander_path = args.resource_path
            else:
                commander_path = copy_dst + "/docker-files"
                get_resources(resource, copy_dst)
            
            if args.target == 'linux':
                extract_file_to_folder(commander_path + "/Commander_linux_x86_64*.tar.bz", copy_dst + "/temp_extract_folder")
                shutil.copytree(copy_dst + "/temp_extract_folder/commander", copy_dst + "/commander")
            elif args.target == 'macos':
                extract_file_to_folder(commander_path + "/Commander_*.zip", copy_dst + "/temp_extract_folder")
                shutil.copytree(copy_dst + "/temp_extract_folder/Commander.app", copy_dst + "/commander")
            else:
                extract_file_to_folder(commander_path + "/Commander_*.zip", copy_dst + "/temp_extract_folder")
                source_path = os.path.join(copy_dst, "temp_extract_folder", "Simplicity Commander")
                destination_path = os.path.join(copy_dst, "commander")
                shutil.copytree(source_path, destination_path)
            shutil.rmtree(copy_dst + "/temp_extract_folder", ignore_errors=True)
        print()

    for resource in common_resources:
        if resource['download_name'] == 'Unify Debian packages' and args.unify_path is not None:
            for extract in resource["extract_details"]:
                shutil.copy(args.unify_path, copy_dst + "/" + extract["extract_dst"])
        elif resource['download_name'] == 'Matter Bridge Binaries'and args.unify_matter_bridge_path is not None:
            get_local_resources(resource, copy_dst, args.resource_path + "/bridge_binaries.zip")
        elif resource['download_name'] == 'EED Debian' and args.resource_path is not None:
            get_local_resources(resource, copy_dst, args.resource_path + "/eed.zip")
        else:
            get_resources(resource, copy_dst)
        print()

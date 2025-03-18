# SPDX-License-Identifier: ZLib
# This script can be used to attach and detach a device to your WSL2 distribution

[CmdletBinding()]
param(
    [Parameter(Mandatory= $false, HelpMessage = "Busid of the device you want to bind")]
    [Alias("b")]
    [string]$Busid,

    [Switch]$Attach,
    [Switch]$Detach,
    [Switch]$List
)

# Unblock the script file
Unblock-File -Path $MyInvocation.MyCommand.Path

Function List-devices {
    usbipd list
}

Function Attach-device {
    param(
        [string]$Busid
    )
    Write-Host "Attaching device $Busid"
    usbipd bind --busid $Busid --force
    usbipd attach --wsl --busid $Busid --auto-attach
}

Function Detach-device {
    param(
        [string]$Busid
    )
    Write-Host "Detaching device $Busid"
    usbipd detach --busid $Busid
    usbipd unbind --busid $Busid
}

if ($Attach -or $Detach -or $List)
{
    if($List)
    {
        List-devices
    }
    if ($Detach)
    {
        if ($Busid)
        {
            Detach-device -Busid $Busid
        }
        else
        {
            Write-Host "Busid not specified"
        }
    }
    if ($Attach)
    {
        if ($Busid)
        {
            Attach-device -Busid $Busid
        }
        else
        {
            Write-Host "Busid not specified"
        }
    }
}
else
{
    Write-Host "No argument specified. Use -Attach, -Detach or -List"
    Write-Host 'Ex: ./wslusb.ps1 -b "5-3" -Attach'
}
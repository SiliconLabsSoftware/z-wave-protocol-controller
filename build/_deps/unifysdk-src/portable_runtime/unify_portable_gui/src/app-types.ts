export type AppState = {
    Status: Status,
    PreRequisitesCheck: PreRequisitesCheck,
    ProtocolList: ProtocolList,
    Frameworks: Frameworks,
    Applications: Applications,
    Emulator: Emulator,
    MultiProtocols: boolean,
    ServiceList: any[],
    DeviceList: string[],
    ErrorMessage: string,
    StatusList: any[],
    Conflicts: any
}

export enum BuildStatus {
    Init,
    Waiting,
    Pass,
}

export type PreRequisitesCheck = {
    Docker: BuildStatus,
    DockerCompose: BuildStatus,
    UnifyLibrary: BuildStatus,
}

export enum Status {
    PreRequisitesCheck,
    Protocol,
    Framework,
    Configuration,
    Review,
    Running,
    Error
}

export type ProtocolList = {
    "Z-Wave": ZWaveProtocolItem
}

export type Frameworks = {
    NAL: boolean,
    "Image-Provider": boolean,
    GMS: boolean,
    UPVL: boolean
}

export type Applications = {
    UMB: boolean
}

export type Emulator = {
    "EED" : EmulatorItem
}

export type EmulatorItem = {
    Enabled: boolean
    DeviceType: string | DeviceTypeList
}

export type ProtocolItem = {
    Enabled: boolean,
    Device: string,
    IsManual: boolean
}

export type ZWaveProtocolItem = ProtocolItem & {
    RF: Frequency;
}

export enum DeviceTypeList {
    DimmableLight = "dimmablelight",
    OnOffLight = "onofflight",
    DoorLock = "doorlock",
    OnOffLightSwitch = "onofflightswitch",
    LightSensor = "lightsensor",
    OccupancySensor = "occupancysensor",
    OnOffPlugInUnit = "onoffpluginunit",
    DimmablePlugInUnit = "dimmablepluginunit",
    ExtendedColorLight = "extendedcolorlight",
    WindowCovering = "windowcovering",
    Thermostat = "thermostat",
    FlowSensor = "flowsensor",
    AllClustersDevice = "allclustersdevice"
}

export enum Frequency {
    EU = "EU",
    US = "US",
    AU_NZ = "ZU_NZ",
    HK = "HK",
    MY = "MY",
    IN = "IN",
    JP = "JP",
    RU = "RU",
    IL = "IL",
    KR = "KR",
    CN = "CH",
    US_LR1 = "US_LR1",
    US_LR2 = "US_LR2",
    US_LR3 = "US_LR3",
    EU_LR1 = "EU_LR1",
    EU_LR2 = "EU_LR2",
    EU_LR3 = "EU_LR3"
}
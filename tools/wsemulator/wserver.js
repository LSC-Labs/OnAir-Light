/**
 * WebSocket emulator for OnAirLight messages
 * i.E. use Postman to simulate the calls
 * see: https://blog.postman.com/set-up-a-websockets-server-in-node-js-postman/
 * 
 * to receive the objects you have to send Json requests from Postman like:
 * { "command": "status" }
 * 
 */

console.log("[I] Starting WebSocket Emulation Server for OnAir Hardware");
console.log("[I] You can connect to ws://localhost (port is 8080)");
console.log("[I| Send your requests to this port with json requests like {\"command\": \"status\"}");
console.log("[I] ..use i.E. Postmand with websock request");

// #region setup the websocket server and default Websocket functions
// For authentication - simulate Token - has to be the same as in the web - page...
var AccessToken="JoWaschlEmulator";

const WebSocket = require("ws");

const wss = new WebSocket.Server({
    port: 8080
});

/// Send message to all clients
wss.broadcast = function broadcast(oJsonData) {
    wss.clients.forEach(function each(client) {
        if (client.readyState === WebSocket.OPEN) {
            client.send(JSON.stringify(oJsonData));
        }
    });
};


/// Send a payload package to all receivers
function sendPayload(strCommand, strDataType, oPayload, bWithToken) {
    let oData = {
        "command": strCommand,
        "data":    strDataType,
        "payload": oPayload
    };
    if(bWithToken === true) oData.auth = AccessToken
    console.log("[I] sending data on websocket...");
    console.log(oData)
    wss.broadcast(oData);
}


/// Register event handler on the websocket
wss.on('connection', function connection(ws) {
    ws.on("error", () => console.log("[W] WebSocket Error - Assume a client is disconnected."));
    ws.on('message', function incoming(message) {
        let obj = JSON.parse(message);
        console.log("[I] Request command '" + obj.command +"' received...") ;
        let strCommand = obj.command ? obj.command.toLowerCase() : "no-command";
        switch (strCommand) {

            case "getstatus":
                sendStatus();
                break;

            case "getconfig":
                sendConfig();
                break;

            case "saveconfig":
                saveConfig(obj.payload);
                break;

            case "getbackup":
                backupConfig();
                break;

            case "gettime":
                sendTime();
                break;

            case "settime":
                setTime(obj.payload);
                break;

            case "getfile":
                sendFile(obj.payload);
                break;

            case "getfilelist":
                sendFileList();
                break;

            case "scanwifi":
                sendWiFiList();
                break;

            case "scanrf433":
                sendRF433ScanCode();
                break;
    
            case "reboot":
                console.log(" ----- REBOOTING -----");
                break;

            default:
                console.log("[W] Unknown command " + strCommand);
                console.log(obj);
                break;
        }
    });
});



//#endregion


// #region Config and Status Data emulators

var oConfigData = {
    "hostname": "lsc-onair01",
    "adminpwd": "admin-geheim",
    "autorestart": "86400",
    "wifi": {
        "bssid": "aa:bb:33:22:11",
        "ap_ssid": "LSC-OnAir",
        "ap_mode": "0",
        "ssid": "WiFiOnICE",
        "wifi_pwd": "JoJoBaOel",
        "offtime": "90", 
        "hostname": "Air-Node"   
    },
    "app": {
        "logConsole": "false",
        "traceMode":  "false",
    },
    "onair": {
        "priority": "mic",
        "oncam": "wave",
        "onmic": "on"
    },
    "mqtt" : {
        "host": "mqtt-host",
        "port": "",
        "topic": "",
        "user": "",
        "passwd": ""
    },
    "rf433": {
            "enabled": true,
            "msgs": [
                { "on": 4333356,"msg": 1, "type":1 },
                { "on": 4333362,"msg": 0, "type":0 }
            ],
          }
};


function backupConfig() {
    sendPayload("backup","config",oConfigData,true);
}

function sendConfig() {
    sendPayload("update","config",oConfigData,true);
    // wss.broadcast(stats);
}
function saveConfig(oCfg) {
    console.log("[I] Saving config data...")
    console.log(oCfg);
    oConfigData = oCfg;
}


function sendStatus() {
    var stats = {
        "prog_name": "OnAir Light",
        "prog_version": "0.5.0.12-D",
        "uptime": "34:42:43",
        "starttime": 74,
        "now": 124963480,
        "full_ver": "SDK:2.2.2-dev(38a443e)/Core:3.2.0-dev=30200000/lwIP:STABLE-2_1_3_RELEASE/glue:1.2-70-g4087efd/BearSSL:5166f2b",
        "chip_id": "82dbdf",
        "cpu_clock": 80,
        "core_ver": "1a13ab95",
        "flash_size": 4194304,
        "flash_real_size": 4194304,
        "heap_free": 28208,
        "heap_max": 27192,
        "sketch_size": 573264,
        "sketch_free_size": 2572288,
        "fs_total": 1024000,
        "fs_used": 32768,
        "fs_block_size": 8192,

        "wifi": {
            "hostname": "OnAirSimulator",
            "accesspoint": true,
            "stationmode": false,
            "isConnected": true,
            "startTime": 6234,
            "ssid": "OnAir-ebcd45cf",
            "dns": "192.168.4.1",
            "mac": "26:D7:EB:CD:45:CF",
            "ip": "192.168.4.1",
            "gateway": "192.168.4.1",
            "netmask": "255.255.255.0",
            "rssi": -75
        },
        "app": {
            "rebootPending": 0,
            "ButtonPressed": "0",
            "RestartWiFi": "0",
            "DebugMode": "0"
        },
        "onair": {
            "isMicOn": false,
            "isCamOn": false,
            "timeout": 0,
            "clients": [
            {
                "client": "192.168.132.139",
                "isCamOn": false,
                "isMicOn": false,
                "lastUpd": 11924258
            },
            {
                "client": "RF433",
                "isCamOn": false,
                "isMicOn": false,
                "lastUpd": 728968
            }
            ]
        },
        "mqtt": {
            "isEnabled": true,
            "isConnected": false,
            "started": 33445,
            "disconTS": 33467,
            "disconReasonRC": 4,
            "disconReason": "stupid counterpart"
        },
        "bat": {
            "power": 4.17,
            "available": true,
            "raw": 973
          },
          "rf433": {
            "enabled": true,
            "msgs": [
                { "on": 4333356,"msg": 1, "type":0 },
                { "on": 4333362,"msg": 0, "type":1 }
            ],
          }
    };
    let oMissing = {
        "ssid": "emuSSID",
        "dns": "8.8.8.8",
        "mac": "EM:44:11:33:22",
        "ip": "192.168.2.2",
        "gateway": "192.168.2.1",
        "netmask": "255.255.255.0"
    };
    sendPayload("update","status",stats);
}

function sendRF433ScanCode() {
    let nMin = 0xff0000;
    let nMax = 0xfffff0
    let nNum = Math.floor(Math.random() * (nMax - nMin + 1) + nMin);
    sendPayload("update","rf433code", { on: nNum });
}

//#endregion

// #region Time and Status emulators

function sendTime() {
    console.log("[I] Sending time...");
    sendPayload("update","time",{ "Time": Math.floor((new Date).getTime() / 1000 )});
}

function setTime(oData) {
    console.log("[I] Setting local time to : " + oData.Time);
    oTime = new Date();
    oTime.setTime(oData.Time * 1000);
    console.log(oTime);
}

// #endregion

// #region Directory - and File emulators

function sendFileList() {
    let oPayloadData = [
        { "size": 1024, "name": "config.json" },
        { "size": 2048, "name": "data.txt" }
    ];
    sendPayload("file","dir",oPayloadData);
}

function sendFile(strFileName) {
    let oPayloadData = { 
        "name": strFileName
    };

    if(strFileName == "config.json") {
        oPayloadData["status"] = "OK";
        oPayloadData["content"] = configdata;
    } else if(strFileName == "data.txt" ) {
        oTextFile = "Hello World\nDas ist ein Textfile\n\nmit einem Testinhalt";
        oPayloadData["status"] = "OK";
        oPayloadData["content"] = oTextFile;
    } else {
        oPayloadData["status"] = "ERR";
    }
    sendPayload("file","content",oPayloadData);
}

// #endregion

// #region WiFi emulators

function sendWiFiList() {
    let oPayload = [{
                        "ssid": "Company's Network",
                        "bssid": "4c:f4:39:a1:41",
                        "rssi": "-84"
                    },
                    {
                        "ssid": "Home Router",
                        "bssid": "8a:e6:63:a8:15",
                        "rssi": "-42"
                    },
                    {
                        "ssid": "SSID Shown Here",
                        "bssid": "8a:f5:86:c3:12",
                        "rssi": "-77"
                    },
                    {
                        "ssid": "Great Wall of WPA",
                        "bssid": "9c:f1:90:c5:15",
                        "rssi": "-80"
                    },
                    {
                        "ssid": "Not Internet",
                        "bssid": "8c:e4:57:c5:16",
                        "rssi": "-87"
                    }
                ];
    sendPayload("update","ssidlist",oPayload);

}
// #endregion


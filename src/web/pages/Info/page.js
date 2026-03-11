/**
  @PageBuilder:register(InfoPage,CInfoPage)
 */

class CInfoPage extends CPageHandler {
    /**
     * 
     * @param {CAPP} oApp 
     */
    constructor(oApp) {
        // don't use this, before initialized !
        super("info",new CElement("#InfoPage"),oApp.Config);
        this._App = oApp;
       // this.Translator = new CTranslator();
    }

    LocalVars = new CVarTable();

    
    // #region Update Status on a regular base
    disposePage(pView, pApp) {
        super.disposePage(pView, pApp);
        this.RefreshTimer = clearInterval(this.RefreshTimer);
        this.RefreshTimer = null;
    }

    preparePage(pView, pApp) {
        super.preparePage(pView, pApp);
        // Set an interval to refresh the status infos every 5 seconds...
        this.RefreshTimer = setInterval( this.refreshStatus.bind(this, pView, pApp), 5000);
    }
    
    refreshStatus(pView, pApp) {
        pApp.requestActStatus();
    }

    /**
     * Triggered, when a new status message is received from the web socket.
     * @param {CView} pView 
     * @param {CApp} pApp 
     * @returns false to avoid loadPageConfig is called again (!) and the game starts again...
     */
    onUpdateStatusReceived(pView, pApp) {
        this.loadPageConfig(pView, pApp);
        return(false)
    }
    // #endregion

    /**
     * 
     * @param {*} pView 
     * @param {*} pApp 
     * @returns 
     * @overload
     */
    loadPageConfig(pView, pApp) {
        let oStatus = pApp.DeviceStatus;

        this.LocalVars.setVars(pApp.tVars);
        this.LocalVars.setVars(oStatus);
       
        // set the root element infos - no prefix...
        this.setConfigValues(pView,oStatus);
        for(let strName in oStatus) {
            let oData = oStatus[strName];
            if(oData && Utils.isObj(oData)) {
                this.setConfigValues(pView,oData,strName);
            }
        }
        let oLight = pView.gel(".OnAirLight");
        if(this.loadClientTable(pView,oStatus)) {
            oLight.ac("OnAirActive");
        } else {
            oLight.rc("OnAirActive");
        }
        pView.translate();
        return(oStatus);
    }

    /**
 * show the current client list in the status page
 * @param {*}  oClientList 
 */
    loadClientTable(pView,oDeviceStatus) {
        let bIsActive = false;
        let oClientList;
        if(oDeviceStatus && oDeviceStatus.onair) oClientList = oDeviceStatus.onair.clients;

        let oTranse = new CTranslator();
        let oTable = document.getElementById("client-table");
        const oNowMS = Date.now();
        const oDevActTimeMS = oNowMS - oDeviceStatus.now;
        const oDevActStartMS = oDevActTimeMS - oDeviceStatus.starttime;

        if(oTable) {
            oTable.innerHTML = "";
            if(!oClientList || oClientList.length == 0) {
                let oRow = oTable.insertRow(-1);
                ; let oCell = oRow.insertCell(0);
                EC(oRow.insertCell(0)).i18n("Info.noClients");
                // oTranse.translate(oCell,undefined,this.LocalVars);
                // oCell.innerHTML = oTranse.translateI18n("Info.noClients","---",this.LocalVars); // "No clients connected...";
            } else {
                for(let i=0; i < oClientList.length; i++) {
                    if(oClientList[i].isCamOn || oClientList[i].isMicOn) bIsActive = true;

                    let oRow = oTable.insertRow(-1);
                    EC(oRow.insertCell(0))
                        .ac("clientAddress")
                        .html(oClientList[i].client);            
                    EC(oRow.insertCell(1)).i18n(oClientList[i].isCamOn ? "Info.cam" : "Info.none");
                    EC(oRow.insertCell(2)).i18n(oClientList[i].isMicOn ? "Info.micro" : "Info.none");
                    const nLastUpd = oDevActStartMS + oClientList[i].lastUpd;
                    const oUpdDate = new Date(nLastUpd);
                    EC(oRow.insertCell(3)).setText(oUpdDate.toLocaleTimeString());
                }
            }
        }
        return(bIsActive);
    }

}
// #region RF433 Module definition (script)

/** 
 * RF433 names and constants
 */
const RF433 = {
    "DIALOG"            : "editRF344Dialog",
    "SCAN"              : "scanRF433",
    "ONAIR_CAMERA"      : 0,
    "ONAIR_MICRO"       : 1,
    "ONAIR_DEVICE_ON"   : 1,
    "ONAIR_DEVICE_OFF"  : 0
}

/**
 * Dialog Class to edit the RF433 remotes
 */
class CRF433Dialog extends CDialog {
    /**
     * Constructor with or without data.
     * If no data, it is only a frame to manipulate the dialog data
     * like from the scanRF433 result.
     * @param {object} oData 
     * {
     *      mode: "edit"    // see language file
     *      on: 12345       // code of the remote
     *      msg: 0/1        // cam or mic
     *      cmd: 0/1        // command on / off
     *      srcID:  undefined/strID // If edit, the id of the row to be edited..
     *      
     * }
     */
    constructor(oData) {
        super("#" + RF433.DIALOG, { cancelBtn:true });
        if(oData) {
            let i18nPrefix = "RF433." + oData.mode;
            try { 
                this.gel(".dialogHeader").i18n(i18nPrefix +  ".title");
                // Keep data at last... if srcID is undefined, data is a query, not a set !
                this.gel("#dlg_on")
                        .reset()
                        .setValue(oData.on ?? "")
                        .on("keyup",this.checkValidity.bind(this),"textKeyUp")
                        .data("srcID",oData.srcID);

                this.gel("#dlg_msg").setValue(oData.msg ?? RF433.ONAIR_CAMERA); 
                this.gel("#dlg_cmd").setValue(oData.cmd ?? RF433.DEVICE_ON);
            } catch {}        
            this.checkValidity();
        }
    }

    /**
     * To set a random code (number string),
     * so the validation stays valid, when the dialog is inactive
     * @param {string} nCode 
     */
    setScanCode(nCode) {
        if(nCode) {
            this.gel("#dlg_on").setValue(nCode);
            this.checkValidity();
        }
    }

    /**
     * Checks the validity of the Dialog code field and enables/disable the save btn.
     */
    checkValidity() {
        EC("#setRF433Btn").enable(EC("#dlg_on").isValid());
    }
}

/***
 * 
 * RF433 Settings page
 * 
 * >> Tell the page builder to register this class as handler for page with id "RF433Page"
 * @PageBuilder:register(RF433Page,CRF433Page)
 * 
 */
class CRF433Page extends CPageHandler {

    /**
     * @param {CAPP} oApp 
     */
    constructor(oApp) {
        super("rf433",new CElement("#RF433Page"),oApp.Config);
        this._App = oApp;
    }

      /**
     * show the RF433 Control Code Dialog (edit / add)
     * The dialog will be translated before it is shown...
     * At least the mode has to be set to edit or new, so
     * the translation of the dialog is correct.
     * If you want to use other modes, update the language files.
     * @param {object} oOptions 
     */
    _showDialog(oOptions) {
        if(Utils.isObj(oOptions)) {
            let oDlg = new CRF433Dialog(oOptions);
            this._App.Translator.translate(oDlg.getBase())
            .then(e => {
                oDlg.showModal();
            });
        }
    }

    /**
     * entry point to load config data
     * @param {CView} pView 
     * @param {CAPP} pApp 
     * @returns the config of this page
     * @overload
     */
    loadPageConfig(pView, pApp) {
        super.loadPageConfig(pView,pApp);
        this._pCurrentView = pView;
        this._pCurrentApp  = pApp;
        let oConfig = this.getConfig(false);

        if(pApp && oConfig && oConfig.msgs) {
            for(let oMsg of oConfig.msgs) {   
                this._appendRFCode(pView,oMsg.on,oMsg.msg,oMsg.type);
            }
        }
        return(oConfig);
    }

    saveConfigValues(pView,oCfg,strPrefixName) { 
        if(pView && Utils.isObj(oCfg)) {
            this.saveElementToConfig(pView.sel("[data-cfg = 'enabled']"),oCfg,strPrefixName);
            oCfg.msgs = [];
            pView.gelAll("tr").forEach(oRow => {
                let oMsg = {}
                oMsg.on = oRow.data("on");
                oMsg.msg = oRow.data("msg");
                oMsg.type = oRow.data("cmd");
                if(oMsg.on) oCfg.msgs.push(oMsg);
            });

        }
    }

    /**
     * Generates a unique id of the row with this (remote) code...
     * @param {integer} nCode 
     * @returns 
     */
    _getRowID(nCode) {
        return("rf433_msg_" + nCode);
    }
    
    /**
     * Appen a row to the remote table
     * @param {CView} pView 
     * @param {string/integer} nOn 
     * @param {string/integer} nMsg 
     * @param {string/integer} nCmd 
     */
    _appendRFCode(pView, nOn,nMsg,nCmd) {
        let oTableBody =  pView.gel("#knownCodes");    
        if(oTableBody.hasBase()) {
            let strID = this._getRowID(nOn);
            let oRow = this._pCurrentView.gel("#" + strID);
            if(oRow.hasBase()) oRow.html("");
            else {
                oRow = oTableBody.cce("tr");
                oRow.ac("rf433-msgRow")
            }
            this._setRowData(oRow,nOn,nMsg,nCmd);
        }
    }

    /**
     * Set the data of this row.
     * Already existing data will be replaced (even the id of the row)
     * The row will be translated after creating the elements
     * @param {CView}  pView 
     * @param {CElement} oRow 
     * @param {integer} nOn 
     * @param {integer} nMsg 
     * @param {integer} nCmd 
     */
    _setRowData(oRow, nOn, nMsg, nCmd) {

        let strDevice = (nMsg    == RF433.ONAIR_CAMERA    ? "cam" : "mic");
        let strCmd    = (nCmd    == RF433.ONAIR_DEVICE_ON ? "on" : "off");
        let strID     = this._getRowID(nOn);
        oRow.id(strID);
        oRow.html("");
        oRow.data("on",nOn).data("msg",nMsg).data("cmd",nCmd);
        oRow.cce("td").attr("title","" + nOn).html("0x" + (nOn.toString(16)));
        oRow.cce("td").i18n("RF433." + strDevice).html(strDevice);
        oRow.cce("td").i18n("RF433." + strCmd).html(strCmd);

        oRow.cce("td")
            .cce("button")
            .id("edit_" + strID)
            .attr("class","btn btn-primary btn-sm pull-right rf433-enabled")
            .attr("style","width:5em")
            .data("i18n","RF433.edit")
            .on("click",this._editReceiver.bind(this),strID);

        oRow.cce("td")
            .cce("button")
            .id("del_" + strID)
            .attr("class","btn btn-danger btn-sm pull-right rf433-enabled")
            .attr("style","width:5em")
            .data("i18n","RF433.del")
            .on("click",this._deleteReceiver.bind(this),strID);

        this._App.Translator.translate(oRow);
    }

    /**
     * Edit the receiver.
     * Triggered by the generated "Edit" button of a single row
     * @param {PointerEvent} pPointerEvent 
     */
    _editReceiver(pPointerEvent) {
        let strSrcID = EC(pPointerEvent.currentTarget).evtArgs();
        let oElement = this._pCurrentView.gel("#" + strSrcID)
        this._showDialog({
            "mode": "edit",
            "srcID": strSrcID,
            "on" : oElement.data("on"),
            "msg" : oElement.data("msg"),
            "cmd": oElement.data("cmd"),
            "pView": this._pCurrentView,
            "pPageHandler": this,
            "oElement": oElement
        }); 
    }

     /**
     * Delete button of a row is pressed...
     * @param {PointerEvent} oPointerEvent 
     */
    _deleteReceiver(oPointerEvent) {
        let oElement = EC(oPointerEvent.currentTarget);
        let strRFID  = oElement.evtArgs();
        let oRow = this._pCurrentView.sel("#" + strRFID );
        if(oRow) { oRow.remove(); this._bIsModified = true; }
    }
  
    /**
     * catch the click events from the page and the dialog.
     * Called by the application / view handling
     * @param {CView} pView 
     * @param {HTMLElement} oElement 
     * @param {string} strCmd 
     */
    on(pView,oElement,strCmd) {
        super.on(pView,oElement);
        if(Utils.isElement(oElement)) {
            switch(oElement.id) {
                case "addRF433Btn":
                    this._showDialog({"mode": "add"}); 
                    break;
                case "setRF433Btn": 
                        // use button pressed - button event args are
                        // either the id of row to be edited or empty,
                        // if it should be a new row (add)
                        let oCodeElement = pView.gel("#dlg_on");
                        let nOn  = parseInt(oCodeElement.getValue());
                        let nMsg = parseInt(pView.gel("#dlg_msg").getValue());
                        let nCmd = parseInt(pView.gel("#dlg_cmd").getValue());
                        let strSrcRowID = oCodeElement.data("srcID");
                        let oRow = pView.gel("#" + strSrcRowID);
                        if(oRow.hasBase()) {
                            // If there is a source in place, set the new/org id to the row
                            // So it will be replaced instead of appended when changed.
                            oRow.id(this._getRowID(nOn));
                            this._setRowData(oRow,nOn,nMsg,nCmd);
                        } else {
                            this._appendRFCode(pView,nOn,nMsg,nCmd);
                        }
                        this._bIsModified = true;
                        this.closeDialog(pView,oElement);
                        break;
                case "scanRF433": 
                    this._App.sendSocketCommand(RF433.SCAN);
                    break;
            } 

            // Close action for modal dialog ?
            if(oElement.dataset.dismiss == "modal") {
                (new CRF433Dialog()).setScanCode("9")
                this.closeDialog(pView,oElement);
            } 
        }
    }

    /**
     * listen on the web socket message and check if the message is a "rf433code" message.
     * @param {CView} pView 
     * @param {object} oMsg 
     */
    onSocketMessage(pView, oMsg) {
        // Scan result code received...
        // insert it into the modal dialog code field...
        if(oMsg && oMsg.data == "rf433code" && oMsg.payload) {
            let oDlg = (new CRF433Dialog());
            oDlg.setScanCode(oMsg.payload.on)
        }
    }
}

// #endregion

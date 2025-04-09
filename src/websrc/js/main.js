/**
 * Main module for LSC OnAir Control
 * (c) LSC-Labs - P.Liebl 2024
 * Based on an idea of ct in december 2024.
 * 
 */


var version = "0.2";




var AccessToken = "JoWaschlEmulator";
const REBOOT_COMMAND         = "restart";
const SAVE_CONFIG_COMMAND    = "saveconfig";
const FACTORY_RESET_COMMAND  = "factoryreset";
const GET_CONFIG_COMMAND     = "getconfig";
const GET_STATUS_COMMAND     = "getstatus";
const GET_BACKUP_COMMAND     = "getbackup";
const RESTORE_BACKUP_COMMAND = "restorebackup";

/**
 * Empty Status object for Device status
 * As it is a gloabl object, used as property, use only the name without type prefix.
 */
var DeviceStatus = {};

// #region Handling of Configuration Settings and Transfers
// ============================================================
/**
 * Empty Config object for Device configuration
 * As it is a gloabl object, used as property, use only the name without type prefix.
 */
var Config = {

};


/**
 * Get the section by name from the global config.
 * If the section name is "", the global config will be used.
 * Function is recursive, so you can specify also subsection like "wifi.network.info"
 * @param {*} strSectionName    Section Name, with delimited sections by '.'
 * @param {*} oBaseSection      null = based on global oConfig, otherwise starting from this section.
 * @param {*} bCreateIfNotExist true = section will be created if it does not exist in oBaseSection...
 * @returns the section (if exists or created) or undefined if the section does not exists.
 */
function getConfigSection(strSectionName, bCreateIfNotExist, oBaseSection) {
  let oResult = undefined;
  oBaseSection = oBaseSection ? oBaseSection : Config;
  if(strSectionName == "") oResult = Config;
  else if(strSectionName && strSectionName.length> 0) {
    let nLastPosOfSeperator = strSectionName.lastIndexOf(".");
    if(nLastPosOfSeperator > -1) {
      let strBaseSectionName = strSectionName.substring(0,nLastPosOfSeperator);
      strSectionName = strSectionName.substring(nLastPosOfSeperator +1);
      oBaseSection = getConfigSection(strBaseSectionName, bCreateIfNotExist ,oBaseSection);
    }
    if(oBaseSection) {
      if(!oBaseSection.hasOwnProperty(strSectionName) && bCreateIfNotExist) {
        oBaseSection[strSectionName] = {};
      }
      oResult = oBaseSection[strSectionName];
    }
  }
  return(oResult);
}

/**
 * Set a value to an element id, without knowing about the type.
 * Is using the "workhorse" to finish...
 * @param {*} strElementId 
 * @param {*} oValue 
 */
function setValueToElementId(strElementID, oValue) {
  setValueToElement(document.getElementById(strElementID), oValue);
}

/**
 * Set a value to an element, without knowing about the type.
 * Used by config to element functions, they only know the property name.
 * @param {*} oElement 
 * @param {*} oValue 
 */
function setValueToElement(oElement,oValue) {
  if(oElement && oValue != undefined) {
    logDebugEntry(" -> setValueToElement(" + oElement.id + " [type: " + oElement.type + "], " + oValue + ")" );
    if(!oElement.type) oElement.innerText = oValue;
    else {
      switch(oElement.type) {
        case "password"   :
        case "text"       : oElement.value = oValue; break;
        case "select-one" : // Select the option if exists - if not, append...
                            let bExists = false;
                            let strVal = oValue;
                            for(let i=0;i<oElement.options.length;i++) {
                              if(oElement.options[i].value == strVal) {
                                  bExists = true; break;
                              }
                            };
                            if(!bExists) {
                              oElement.innerHTML += "<option value='" + oValue + "'>" + oValue + "</option>"; 
                            }
                            oElement.value = strVal;
                            break;
        case "checkbox"   :
        case "radio"      : // Set the checked radio button to true / false.
                            // if value == false, try to set the partner button prefixed with "no-"... and set to true...
                            oElement.checked = oValue == "1" ? true : false; 
                            // If the element is NOT checked, search the 2nd element (partner)
                            // either by attribute "data-radio" or by "no-"<currentid> and set this element to checked.
                            if(!oElement.checked) {
                              let oNoElem = document.getElementById("no-" + oElement.id);
                              if(oNoElem) oNoElem.checked = true;
                              else {
                                $("#ajaxcontent").find("[name='" + oElement.name + "']").each(function(){
                                  if($(this) != oElement) $(this).attr('checked','checked');
                                });
                              }
                            }
                            break;
      }
    }
  }
}


/**
 * Load element values with properties of a json object
 * @param {*} strPrefixName 
 * @param {*} oJsonObj 
 */
function setPropertiesOf(oJsonObj, strPrefixName) {
  if(oJsonObj) {
    if(!strPrefixName) strPrefixName = "";
    for(let strProp in oJsonObj) {
      let strId = strPrefixName + "_" + strProp;
      setValueToElementId( strId,oJsonObj[strProp]);
    }
  }
}

/**
 * Set the element values based on a config section.
 * each property of the config section will be prefixed with the section name.
 * If an element with the name "<SectionName>_<PropertyName>" exists, the value
 * of this element will be set with the content of the config section.
 * !!! Keep id and property name in sync !!! (case sensitive)
 * @param {*} strCfgSectionName 
 */
function setConfigPropertiesOf(strCfgSectionName) {
  let oCfg = getConfigSection(strCfgSectionName);
  setPropertiesOf(oCfg,strCfgSectionName);
}


/**
 * store the element value into the config section.
 * If a property name is defined, otherwise calculated, based on element id.
 * Element ID is starting with the section name like "wifi_",
 * ID "wifi_hostname" will be stored in oCfg["hostname"].
 * Keep id and config name in sync, also respecting case sensitive !
 * @param {*} oElement Element to be stored from document.getElementBy....
 * @param {*} oCfg     config object.
 * @param {*} strProp  if set, the property in config will be used.
 */
function storeElementValueToConfigPropery(oElement,oCfg,strProp) {
  if(oElement && oCfg) {
    if(!strProp) strProp = oElement.id.split('_').splice(1).join('_');
    console.log("Storing element id " + oElement.id + " type: " + oElement.type + " -  to " + strProp);
    switch(oElement.type) {
      case "password"   :
        case "text"       : oCfg[strProp] = oElement.value; 
                            break;
        case "select-one" : // Try to get the specialized value first (i.E. ssid display is different, but the value to be stored
                            // is in the option field "ssid").
                            // If not in place, use the value of the element
                            let strVal = oElement.value;
                            // if(!strVal) strVal = oElement.value;
                            oCfg[strProp] = strVal;
                            break;
        case "checkbox"   :
        case "radio"      : oCfg[strProp] = (oElement.checked === true ? "1" : "0");
                            break;
    }
  }
}

/**
 * Saves the active content page in "#ajaxcontent" into
 * the config section defined, using the function "storeElementValueToConfig()"
 * @param {*} strSectionName 
 */
function storeElementValuesToConfigProperties(strSectionName) {
  let oCfg = getConfigSection(strSectionName,true);
  $("#ajaxcontent").find("[id]").each(function(){
    let strID = $(this).attr("id");
    if(strID.startsWith(strSectionName + "_")) {
      storeElementValueToConfigPropery(document.getElementById(strID),oCfg);
    }
  });
}

/**
 * Set a config value into the page settings, depending on type...
 * @param {*} oCfg 
 * @param {*} tNames 
 */
function setConfigValuesTo(oCfg, tNames) {
  for(const strID of tNames) {
    let oElement = document.getElementById(strID);
    // Element and property in place ?
    if(oElement && oCfg.hasOwnProperty(strID)) {
      setValueToElement(oElement,oCfg[strID]);
    }
  }
}

/**
 * Get the setting from the page by ID and store it into the config object.
 * Depending on element type in the correct way.
 * @param {*} oCfg 
 * @param {*} tNames 
 */
function storeConfigValuesFrom(oCfg, tNames) {
  for(const strID of tNames) {
    let oElement = document.getElementById(strID);
    if(oElement) {
      switch(oElement.type) {
        case "password"   :
        case "text"       : oCfg[strID] = oElement.value; 
                            break;
        case "select-one" : // Try to get the specialized value first (i.E. ssid display is different, but the value to be stored
                            // is in the option field "ssid").
                            // If not in place, use the value of the element
                            let strVal = oElement.value;
                            // if(!strVal) strVal = oElement.value;
                            oCfg[strID] = strVal;
                            break;
        case "checkbox"   :
        case "radio"      : oCfg[strID] = (oElement.checked === true ? "1" : "0");
                            break;
      }
    }
  }
}

//#endregion

// #region Config Change and Commit handling
// =========================================
/**
 * add a save button to the current active page...
 * function must be valid to be inserted..
 * @param {*} pSaveFunction 
 */
function addSaveButton(pSaveFunction) {
  if(typeof pSaveFunction === 'function') {
    $("<button/>",{
        onclick: pSaveFunction.name + "()",
        class: "btn btn-primary btn-sm pull-right data-modified",
        style: "width:8em"
      }).html("Save")
        .appendTo($("<div/>",{ class:"col-xs-12 col-md-10"})
        .appendTo($("<div/>",{ class:"row form-group" })
        .appendTo($("#ajaxcontent"))));
  }
}

/**
 * Something changed in the config - so inform the user...
 */
function setConfigChanged() {
  $("#commit").fadeOut(200, function() {
      $(this).css("background", "gold").fadeIn(1000);
  });
  document.getElementById("commit").innerHTML = "<h6>You have uncommited changes, please click here to review and commit.</h6>";
  $("#commit").click(function() {
      checkConfigBeforeSave();
      return false;
  });
}

/**
 * Ask the user to accept changes in Config before saving...
 */
function checkConfigBeforeSave() {
  document.getElementById("jsonData").innerText = JSON.stringify(Config, null, 2);
  $("#checkContent").modal("show");
}

/**
 * Content is accepted by user...
 * send it to the node and restart...
 */
function acceptConfigChanges() {
  // Save Config
  showRestartPanel(20);
  sendCommandMessage(SAVE_CONFIG_COMMAND,"",Config);
  $("#commit").fadeOut(1000);
  sendCommandMessage(REBOOT_COMMAND);

  //  inProgress("commit");
}

  
//#endregion

// #region GUI event handler
// =========================================


$("#dismiss, .overlay").on("click", function() {
  $("#sidebar").removeClass("active");
  $(".overlay").fadeOut();
});

$("#sidebarCollapse").on("click", function() {
  $("#sidebar").addClass("active");
  $(".overlay").fadeIn();
  $(".collapse.in").toggleClass("in");
  $("a[aria-expanded=true]").attr("aria-expanded", "false");
});

//#endregion


// #region Common Functions and Modules
// =========================================

/**
 * Writes debug information to console, if debug version is active...
 * @param {*} oData 
 */
function logDebugEntry(oData) {
  if(DeviceStatus.DebugMode == 1) console.log(oData);
}

/**
 * shows / hides an element by it's id...
 * @param {*} strID 
 * @param {*} bShow 
 */
function showElement(strID, bShow) {
  let oElement = document.getElementById(strID);
  if(oElement) oElement.style.display = (bShow ? "block" : "none");
}

/**
 * Download a file to the local device (pc/mac/...)
 * @param {*} oDataObj        // Data to be stored (downloaded)... (JSON Object!!!)
 * @param {*} anchorElement   // Anchor element to use for download click simulation...
 * @param {*} strFileName     // Filename for the stored data
 */
function downloadFile(oDataObj,anchorElement,strFileName) {
  let strData = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(oDataObj, null, 2));
  let dlAnchorElem = document.getElementById(anchorElement);
  dlAnchorElem.setAttribute("href", strData);
  dlAnchorElem.setAttribute("download", strFileName);
  dlAnchorElem.click();
}

function removeModal() {
  $("#restoremodal").modal("hide");
  $("body").removeClass("modal-open");
  $("body").css("padding-right", "0px");
  $(".modal-backdrop").remove();
}

/**
 * Loads a single Progress bar with details by ID
 * @param {*} strID 
 * @param {*} strValue 
 * @param {*} nPercentage 
 */
function loadProgressBar(strID,strValue,nPercentage) {
  let oBarObj = document.getElementById("status_" + strID);
  if(oBarObj) {
    oBarObj.innerHTML = strValue;
    oBarObj.style.width = nPercentage + "%";
    let strColor = "success";
    if(nPercentage > 75) strColor = "danger";
    else if(nPercentage > 50 ) strColor = "warning";
    oBarObj.className = "progress-bar progress-bar-" + strColor;
    oBarObj.title = nPercentage.toFixed(1) + "%";

    oBarObj = document.getElementById("status_" + strID + "_rest");
    if(oBarObj) {
      oBarObj.style.width = (100 - nPercentage) + "%";
      oBarObj.title = (100 - nPercentage).toFixed(1) + "%";
    }
  }

}


/**
 * check the elements value.
 * If strMask is not specified, the data-val attribute will be used.
 * If the strMask is $(IP), then a IPV4 Address mask is used..
 * @param {*} oElement 
 * @param {*} strMask 
 * @returns true if valid...
 */
function checkElementValue(oElement,strMask) {
  let isValid = true;
  if(!strMask) strMask = oElement.getAttribute("data-val");
  if(strMask) {
    isValid = false;
    if(strMask == "$(IP)") {
      strMask = /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/;
    }
    let oRX = new RegExp(strMask);
    isValid = oRX.test(oElement.value);
  }
  return(isValid);
}

/**
 * Checks the value of an element by using "checkElementValue".
 * If the result is NOT Valid, a alert box will be displayed to inform
 * the user and the field gets the focus.
 * Function is used by validate functions, before storing the value finaly
 * @param {*} strID id of the element to check
 * @param {*} strErrorMessage message to explain the element
 * @param {*} bUsePreAlertString Insert a "please select a valid " string to message (Default == true)
 * @returns 
 */
function checkElement(strID,strErrorMessage, bUsePreAlertString) {
  let oElement = document.getElementById(strID);
  let isOK = checkElementValue(oElement);
  if(isOK === false) {
    if(strErrorMessage) {
      alert((bUsePreAlertString === false ? "" : "Please select a valid ") + strErrorMessage) ;
      oElement.focus();
    }
  }
  return(isOK);
}

/**
 * Checks the value of an element by using "checkElementValue".
 * it sets or removes the "inError" class to/from the element.
 * Function is used by interactive GUI.
 * @param {*} oElement 
 * @param {*} strMask 
 */
function validateElement(oElement,strMask) {
  if(checkElementValue(oElement,strMask)) $(oElement).removeClass("inError");
  else $(oElement).addClass("inError");
}



// #endregion

// #region Login / Logout
// =========================================

/**
 * Login Button pressed... check password and user...
 */
function login() {
  let strPassword = document.getElementById("password").value;
  if (strPassword === "emu") {
      // Will not work in prod environment - only for emulation mode...
      $("#signin").modal("hide");
      connectWS();
  } else {
      let strUsername = "admin";
      // let password = document.getElementById("password").value;
      let url = "/login";
      let xhr = new XMLHttpRequest();
      xhr.open("get", url, true, strUsername, strPassword);
      xhr.onload = function(e) {
          if (xhr.readyState === 4) {
              if (xhr.status === 200) {
                  // Login is ok, store the access token and establish the websocket
                  AccessToken = xhr.getResponseHeader("AUTHTOKEN");
                  $("#signin").modal("hide");
                  connectWS();
              } else {
                  alert("Incorrect password!");
              }
          }
      };
      xhr.send(null);
  }
}

function logout() {
  jQuery.ajax({
      type: "GET",
      url: "/login",
      async: false,
      username: "logmeout",
      password: "logmeout",
  })
  .done(function() {
      // If we don"t get an error, but this will not happen,
      // we will get an error - expecting an 401!
  })
  .fail(function() {
      // As expected to get an 401 Unauthorized error! In this case we are successfully
      // logged out and we redirect the user to the start page.
      document.location = "index.html";
  });
  return false;
}

// #endregion

// #region Websocket Initialization and Basic functions
// ====================================================


var oWebSocket = null;                // The Websocket object
var isWSConnectionAvailable = false;  // Is the connection activ and available ?
var nWSConnectionTimerID = 0;         // Avoids multiple timers to be in place...
var gotInitialData = false;           // Did we already received the initial data ?

/**
 * Open the WebSocket Connection and establishes the handlers
 * @returns 
 */
function connectWS() {
  if(!isWSConnectionAvailable) {
    let wsUri = "ws://" + window.location.hostname + "/ws";
    if (window.location.protocol === "https:") {
      wsUri = "wss://" + window.location.hostname + ":" + window.location.port + "/ws";
    } else if (window.location.protocol === "file:" ||
        ["0.0.0.0", "localhost", "127.0.0.1"].includes(window.location.hostname)) {
      wsUri = "ws://localhost:8080/ws";
    }
    
    oWebSocket = new WebSocket(wsUri);
    // Register the handler....
    oWebSocket.addEventListener("message", onSocketMessageReceived);
    oWebSocket.onopen  = function(evt) { onWSConnectionOpened(); };
    oWebSocket.onclose = function(evt) { onWSConnectionClosed(); };

    // Setup the reconnection timer (if needed...)
    // calls this function again, if connection got lost...
    keepWSConnectionOpen();
  }
}

/**
 * Timer setup routine to keep the connection alive
 * Don't forget to delete the timer if you don't need it any longer.
 * In this case, the page will be reloaded, so the timer will be destroyed
 * and no extra closeWS is in place.
 */
function keepWSConnectionOpen() {
  if(nWSConnectionTimerID == 0) {
    if (!isWSConnectionAvailable) {
        nWSConnectionTimerID = setTimeout(connectWS, 2000);
    }
  }
}

/**
 * Send a message via the websocket...
 * This is the only function, that access the websock to send.
 * @param {*} msg 
 */
function sendWebsocket(msg) {
  logDebugEntry("Sending socket message:");
  logDebugEntry(msg);
    if(oWebSocket) oWebSocket.send(msg);
}

/**
 * Prepares the message with a command, token and if defined, a type.
 * The message itself will be stored in "payload" property.
 * @param {*} strCommand 
 * @param {*} strType 
 * @param {*} oData 
 */
function sendCommandMessage(strCommand,strType,oData) {
  let oMsg = {
    "command": strCommand,
    "token"  : AccessToken
  }
  if(strType) oMsg.type = strType;
  if(oData)   oMsg.payload = oData;
  sendWebsocket(JSON.stringify(oMsg));
}

// #endregion  

// #region Websocket Callback functions (onXXXX)...
// =========================================

/**
 * A connection has been established and is open now...
 */
function onWSConnectionOpened() {
  isWSConnectionAvailable = true;
  $("#ws-connection-status").slideUp();
  if (!gotInitialData) {
    sendCommandMessage(GET_STATUS_COMMAND);
    sendCommandMessage(GET_CONFIG_COMMAND);
    gotInitialData = true;
  }
}

/**
 * The connection has been closed... so update the status
 * Don't reconnect, as this will be done be a timer (keepWSConnectionOpen) !
 */
function onWSConnectionClosed() {
  isWSConnectionAvailable = false;
  $("#ws-connection-status").slideDown();
}


/**
 * A message received... split up into the message class (Update/Backup/Error)
 * and hand over to the class specific functions
 * @param {*} oMsg 
 */
function onSocketMessageReceived(oMsg) {
  let oMsgData = JSON.parse(oMsg.data);
  if (oMsgData.hasOwnProperty("command")) {
    switch (oMsgData.command) {
      case "update": // Update received
        onUpdateMessageReceived(oMsgData);
        break;

      case "backup": // Backup received
        onBackupMessageReceived(oMsgData);
        break

      case "error": // Error received
        onErrorMessageReceived(oMsgData);
        break;

      default:
        break;
    }
  }
}

/**
 * Backup / Download Message Class handler...
 * @param {*} oMsg 
 */
function onBackupMessageReceived(oMsg) {
  if(oMsg) {
    switch(oMsg.data) {
      case "config":  // A backup request for config received...
        downloadFile(oMsg.payload,"downloadSet","OnAirLight-settings.json")
        break;
    }
  }
}

/// Refresh underlying data objects and the GUI's if available.
/**
 * Update Class handler
 * @param {*} oMsg 
 */
function onUpdateMessageReceived(oMsg) {
  if(oMsg) {
    switch(oMsg.data) {

      case "status":  // A new status received
        DeviceStatus = oMsg.payload;
        break;

      case "config":
        Config = oMsg.payload;
        break;
      
      case "ssidlist":
        insertFoundNetworks(oMsg.payload);
        break;

      case "rf433code":
        RF433Dialog.setScanCode(oMsg.payload);
        // insertRF433Code(oMsg.payload);
        break;

    }
    refreshContentPage("#" + oMsg.data + "content");
  }
}

/**
 * An error message received on the websocket 
 * Inform the user about and log out
 * @param {*} oMsg 
 */
function onErrorMessageReceived(oMsg) {
  let strMessage = "ERROR: " + oMsg.data + "\n\"" + oMsg.payload.command + "\" - " + oMsg.msg;
  alert(strMessage);
  switch(oMsg.data) {
    case "401": // Access denied message
      logout();
      break;
    default:
  }
}

// #endregion

/**
 * Update the content pages - if a handler is in place
 * If a new data object comes in place - i.E. in cause of a refresh timer
 * or if another user has updated the settings...
 * @param {*} strPageID 
 */
function refreshContentPage(strPageID) {
  switch(strPageID) {
    case "#statuscontent":
      loadStatusPage();
      break;
  }
}

function adjustContentAttributes(oTarget,strPrefix) {

  if(oTarget && oTarget.length > 0) {
    
    // insert validate function if not already in place...
    oTarget.find("[data-val]").each(function(nIdx) {
      if(!$(this).attr("onkeyup")) $(this).attr("onkeyup","validateElement(this)");
    });

    // Prepare the prefix to be used for ID
    if(!strPrefix) strPrefix = "";
    if(strPrefix.length > 1) strPrefix = strPrefix + "_"; 
    oTarget.find("[data-id]").each(function() {
      if(!$(this).attr("id")) {
        $(this).attr("id", strPrefix + $(this).attr("data-id"));
      }
    });
    oTarget.find("[data-cfg]").each(function() {
      if(!$(this).attr("id")) {
        $(this).attr("id", strPrefix + $(this).attr("data-cfg"));
      }
    });
  }
}

// Switch to content page.
// Page is hosted by container id "#ajaxcontent"
function switchToContentPage(strPageID, strTargetID) {
  $("#dismiss").click();
  $(".overlay").fadeOut().promise().done(function() {
    let oPageContent = $(strPageID).html();
    let oTarget = $(strTargetID ? "#" + strTargetID : "#ajaxcontent");
    oTarget.html(oPageContent).promise().done(function() {
      strPrefix = strPageID.substring(1);
      strPrefix = strPrefix.substring(0,strPrefix.length - "content".length);
      adjustContentAttributes(oTarget,strPrefix);
      // disable elements for changed data...
      // $("." + strPageID + "-changed").prop("disabled",false);
      // call the loader...
      // ToDo: Implement a alias syntax...
      //  function = windows["name"];
      //  if typeof function === function - call function()
      switch (strPageID) {

        case "#statuscontent":
          loadStatusPage();
          break;

        case "#devicecontent":
          loadDevicePage();
          break;

        case "#networkcontent":
          loadNetworkPage();
          break;

        case "#onaircontent":
          loadOnAirPage();
          break;

        case "#backupcontent":
          loadBackupPage();
          break;
        
        case "#feedbackcontent":
          loadFeedbackPage();
          break;
        
        case "#mqttcontent":
          loadMqttPage();
          break;

        case "#rf433content":
          loadRF433Page();
          break;
      }
      
      $("[data-toggle=\"popover\"]").popover({
        container: "body"
      });
      $(this).hide().fadeIn();
    });
  });
}
//#endregion




// #region Status content page
// =========================================

/**
 * Register page select handler 
 */
$("#status").on("click", (function() {
  switchToContentPage("#statuscontent");
  // Ask for a new status... (refresh is included...)
  sendCommandMessage(GET_STATUS_COMMAND);
  return false;
}));

/**
 * show the current client list in the status page
 * @param {*}  oClientList 
 */
function loadClientTable(oClientList) {
  let oTable = document.getElementById("status_client-table");
  

  const oNowMS = Date.now();
  const oDevActTimeMS = oNowMS - DeviceStatus.now;
  const oDevActStartMS = oDevActTimeMS - DeviceStatus.starttime;

  if(oTable) {
    /*
    while(oTable.rows.length > 1) {
      oTable.deleteRow(1);
    }
      */
    oTable.innerHTML = "<caption>Client list</caption>";
    if(!oClientList || oClientList.length == 0) {
      let oRow = oTable.insertRow(-1);
      let oCell = oRow.insertCell(0);
      oCell.innerHTML = "No clients connected...";
    } else {
      for(let i=0; i < oClientList.length; i++) {
        let oRow = oTable.insertRow(-1);
        let oCell = oRow.insertCell(0);
        oCell.className="clientAddress";
        oCell.innerHTML = oClientList[i].client;
        oCell = oRow.insertCell(1);
        oCell.innerHTML = oClientList[i].isCamOn ? "Camera" : "-";
        oCell = oRow.insertCell(2);
        oCell.innerHTML = oClientList[i].isMicOn ? "Microphone" : "-";
        oCell = oRow.insertCell(3);
        const nLastUpd = oDevActStartMS + oClientList[i].lastUpd;
        const oUpdDate = new Date(nLastUpd);
        oCell.innerHTML = oUpdDate.toLocaleTimeString();
      }
    }
  }
}
/**
 * Load the status page fields
 * each element in the panel with the id "status_<fieldname>"
 */
function loadStatusPage() {
  removeModal();
  setPropertiesOf(DeviceStatus,     "status");
  setPropertiesOf(DeviceStatus.wifi,"status");
  setPropertiesOf(DeviceStatus.mqtt,"status");
  setPropertiesOf(DeviceStatus.bat, "status");
  if(DeviceStatus.bat) {
    if(DeviceStatus.bat.power == 0)  {$("#status_power").html("- ");}
    else {$("#status_power").html(DeviceStatus.bat.power.toFixed(2));}
  }
  if(DeviceStatus.onair) {
    loadClientTable(DeviceStatus.onair.clients);
  }

  // TODO: check version property correct ?
  $("#mainver").text(DeviceStatus.version);

  /*
  let nAllocatedHeap = DeviceStatus.heap_max - DeviceStatus.heap_free;
  loadProgressBar("heap",nAllocatedHeap + " Bytes",
                      ((nAllocatedHeap * 100) / DeviceStatus.heap_max));
  */

  loadProgressBar("flash",(DeviceStatus.sketch_size/1024).toFixed(0) + " KB",((DeviceStatus.sketch_size *100) / (DeviceStatus.sketch_free_size + DeviceStatus.sketch_size)))
  // loadProgressBar("fs",(DeviceStatus.fs_used/1024).toFixed(0) + " KB",((DeviceStatus.fs_used * 100) / DeviceStatus.fs_total));
}

//#endregion

// #region Device content page
// =========================================

$("#device").click(function() {
  switchToContentPage("#devicecontent");
  return false;
});

/**
 * Load the device page from config settings
 */
function loadDevicePage() {
  let strHostName = (Config.wifi && Config.wifi.hostname) ? Config.wifi.hostname : undefined;
  setValueToElementId("hostname",strHostName);
  let strAdminPwd = (Config.web && Config.web.httpPasswd) ? Config.web.httpPasswd : undefined;
  setValueToElementId("adminpwd",strAdminPwd);
/*
  let nAutoRestart = (Config.app && Config.app.autorestart) ? Config.app.autorestart : undefined;
  
  setValueToElementId("autorestart-custom",nAutoRestart);
  setValueToElementId("autorestart",nAutoRestart);

  // if value is not same as the restart option, it's custom
  let checkedOption = document.querySelector("#content #autorestart option:checked");
  if(!checkedOption || parseInt(checkedOption.value) != nAutoRestart) {
    $("#autorestart-custom").removeClass("hidden");
    document.getElementById("autorestart").value = "custom";
  }
  $("#autorestart").on("change", function() {
    if (this.value == "custom") {
      $("#autorestart-custom").removeClass("hidden");
    } else {
      $("#autorestart-custom").addClass("hidden");
    }
  });
  */
  }

function validateDevicePage() {
  let isOK = false;
  isOK = checkElement("adminpwd",   "administrator password") &&
         checkElement("hostname",   "hostname");
                      //  &&checkElement("autorestart","restart time");
                      /*
  if(isOK && document.getElementById("autorestart").value == "custom") {
      isOK = checkElement("autorestart-custom", "restart time");
  }
      */
  return isOK;
}

function saveDevicePage() {  
  if(validateDevicePage()) {
    let oWebCfg = getConfigSection("web",true);
    oWebCfg.httpPasswd = document.getElementById("adminpwd").value;
    let oNetCfg = getConfigSection("wifi",true);
    oNetCfg.hostname = document.getElementById("hostname").value;
    /*
    let strVal = document.getElementById("autorestart").value;
    if(strVal == "custom") {
      strVal = document.getElementById("autorestart-custom").value;
    }
    Config.autorestart = parseInt(strVal);
    */
    setConfigChanged();
  }
}

//#endregion

// #region Network content page
// =========================================


/**
 * Scan the current WiFi Area for networks
 * Callback via WebSocket
 * Reset the current password field also...
 */
function scanWifi() {
  sendWebsocket("{\"command\":\"scanwifi\"}");
  document.getElementById("scanwifibtn").innerHTML = "...";
  // document.getElementById("inputtohide").style.display = "none";
  let oNode = document.getElementById("ssid");
  if(oNode) {
    oNode.style.display = "inline";
    while (oNode.hasChildNodes()) {
      oNode.removeChild(oNode.lastChild);
    }
    oNode = document.getElementById("passwd");
    if(oNode) oNode.value = "";
  }
}

function updateBSSID() {
  let oSelect = document.getElementById("ssid");
  document.getElementById("bssid").value = oSelect.options[oSelect.selectedIndex].bssidvalue;
}

function insertFoundNetworks(oListOfSSIDs) {
  let oSelect = document.getElementById("ssid");
  for(let i=0; i < oListOfSSIDs.length; i++) {
    let x = parseInt(oListOfSSIDs[i].rssi);
    let nPercent = Math.min(Math.max(2 * (x + 100), 0), 100);
    let oOpt = document.createElement("option");
    oOpt.value = oListOfSSIDs[i].ssid;
    oOpt.bssidvalue = oListOfSSIDs[i].bssid;
    oOpt.innerHTML =oListOfSSIDs[i].ssid + " (" + nPercent + "%) - " + oListOfSSIDs[i].bssid ;
    oSelect.appendChild(oOpt);
  }
  document.getElementById("scanwifibtn").innerHTML = "Re-Scan";
  updateBSSID();
}

$("#network").on("click", (function() {
    switchToContentPage("#networkcontent");
    return false;
}));


function loadNetworkPage() {
  let oCfg = getConfigSection("wifi",true);
  setConfigValuesTo(oCfg,["ap_mode", "ap_ssid", "ssid", "bssid", "ap_pwd", "wifi_pwd", "dhcp", "ipaddress", "subnet", "dnsip", "gwip","fallback"]);
  updateNetworkPageView();
}

function isAccessPointSelected() {
  return(document.getElementById("ap_mode").checked)
}
function isDHCPSelected() {
  return(document.querySelector("input[name=\"dhcpenabled\"]:checked").value === "1");
}

function updateNetworkPageView() {
  let bAPMode = isAccessPointSelected();

  if(bAPMode) {
    $(".sta-mode").hide();
    $(".ap-mode").show();
  }
  else {
    $(".ap-mode").hide();
    $(".sta-mode").show();
  }

  if(bAPMode) {
    $("#ip_statics").slideDown();
    $("#ip_statics").show();
    $("#ip_services").slideUp();
  } else {
    if(isDHCPSelected()) {
      $("#ip_statics").slideUp();
      $("#ip_services").slideUp();
    } else {
      $("#ip_statics").slideDown();
      $("#ip_statics").show();
      $("#ip_services").slideDown();
      $("#ip_services").show();
    }
  }
 
/*
  document.getElementById("ap_ssid").style.display       = bAPMode ? "block"   : "none";    
  document.getElementById("ap_passwd").style.display     = bAPMode ? "block"   : "none";  
  document.getElementById("ap_hide_area").style.display  = bAPMode ? "block"   : "none";    
  document.getElementById("ssid").style.display          = bAPMode ? "none"    : "block";
  document.getElementById("passwd").style.display        = bAPMode ? "none"    : "block";
  document.getElementById("scanwifibtn").style.display   = bAPMode ? "none"    : "block";    
  document.getElementById("hideBSSID").style.display     = bAPMode ? "none"    : "block";
  document.getElementById("dhcp_area").style.display     = bAPMode ? "none"    : "block"; 
  document.getElementById("fallback_area").style.display = bAPMode ? "none"    : "block"; 
  */
}

/**
 * validate the user GUI entries to be stored.
 * @returns true if settings can be processed
 */
function validateNetworkPage() {
  let isValid = false;
  // Access Point Mode - or STA without DHCP check...
  if (isAccessPointSelected()) {
    // Access Point Mode - need a vlaid ipaddress and subnet
    isValid = checkElement("ap_ssid","SSID")         &&
              checkElement("ipaddress","IP address") &&
              checkElement("subnet","subnet mask");
  } else {
    isValid = checkElement("ssid","SSID") &&
              checkElement("bssid","BSSID");
    if (isValid && !isDHCPSelected()) {
      // DHCP off -> fix address needed
      isValid = checkElement("ipaddress", "IP address") &&
                checkElement("subnet", "subnet mask")    &&
                checkElement("dnsip","DNS address")    &&
                checkElement("gwip","Gateway address");
    }
  }
  return(isValid);
}

/**
 * Save the network settings (into the config file)
 * Validate first, and only if the settings are valid - touch the config
 * @returns 
 */
function saveNetworkPage() {
  // Validate the user settings, before touching the config...
  if(validateNetworkPage()) {
    let oCfg = getConfigSection("wifi",true);
    let bDHCP = isDHCPSelected();
    let bAP   = isAccessPointSelected();
    storeConfigValuesFrom(oCfg,["ap_mode"]);
    if(bAP) {
      storeConfigValuesFrom(oCfg,["ap_ssid", "ap_passwd", "ap_hide", "ipaddress", "subnet"]);
    } else {
      storeConfigValuesFrom(oCfg,["ssid","wifi_pwd","bssid","dhcp","fallback"]);
      if(!bDHCP) {
        storeConfigValuesFrom(oCfg,["ipaddress","subnet","dnsip","gwip"]);
      }
    }
    oCfg.wmode = bAP  ? "1" : "0";
    setConfigChanged();
  }
 
}

//#endregion

// #region RF433 Remote Control Settings
$("#rf433").on("click", (function() {
  switchToContentPage("#rf433content");
  return false;
}));


const ONAIR_CAMERA = 0;
const ONAIR_MICRO  = 1;
const ONAIR_DEVICE_ON  = 1;
const ONAIR_DEVICE_OFF = 0;

var ContentBase = {
  DataId:  "",

  setTargetArea: function(strTargetId) {
    this.Area = strTargetId ? $("#" + strTargetId) : $("#ajaxcontent");
    return(this);
  },

  setDataId: function(strDataId) {
    this.DataId = strDataId ?? "";
    return(this);
  },

  getFullId: function(strDataId) {
    return((this.DataId ? this.DataId + "_" : "") + strDataId);
  },

  getById: function(strDataId) {
    return( $("#" + this.getFullId(strDataId )));
  },

  getConfig: function() {
    return(getConfigSection(this.DataId,true));
  },

  getElementValue: function(oObj) {
    let oResult;
    if(oObj) {
      switch(oObj[0].type) {
        case "password"   :
        case "text"       :                    
        case "select-one" : 
                            oResult = oObj[0].value;
                            break;
        case "checkbox"   :
        case "radio"      : oResult = oObj[0].checked; // === true ? "1" : "0");
                            break;
        default : oResult = oObj[val];
      }
    }
    return(oResult);
  },

  saveConfigValues: function(oArea) {
    let oCfg = this.getConfig();
    let pCurObj = this;
    // Prio 1, all data-cfg settings...
    oArea.find("[data-cfg]").each(function(){
      let strKey = $(this).data("cfg");
      if(strKey && strKey.length > 0) {
        oCfg[strKey] = pCurObj.getElementValue($(this));
      }
    });
  },

  addSaveButton: function(pSaveFunction) {
    let strFunctionName = pSaveFunction;
    if(typeof pSaveFunction === 'function') strFunctionName = pSaveFunction.name;
    $("<button/>",{
        onclick: strFunctionName + "()",
        class: "btn btn-primary btn-sm pull-right data-modified",
        style: "width:8em"
      }).html("Save")
        .appendTo($("<div/>",{ class:"col-xs-12 col-md-10"})
        .appendTo($("<div/>",{ class:"row form-group" })
        .appendTo(this.Area)));
  },
}

var PageBase = {

    load: function(oSource,funcLoadPageData) {
      setPropertiesOf(this.getConfig(),this.DataId);
    },    
}

var DialogBase = {
  loadDialog: function(strSourceId, funcLoadDataCallback,pDialog) {
    if(strSourceId) this.SourceNode = $("#" + strSourceId);
    if(this.SourceNode && this.Area) {
      this.Area.html(this.SourceNode.html()).promise().done(function(){
        adjustContentAttributes(pDialog.Area,pDialog.DataId);
        if(funcLoadDataCallback) funcLoadDataCallback(pDialog);
        pDialog.Area.modal("show");
      });
    }
  }
}

var PageHandler   = $.extend({}, ContentBase, PageBase).setTargetArea("ajaxcontent");
var DialogHandler = $.extend({}, ContentBase, DialogBase).setTargetArea("dialogContent");

var RF433Content = {

  loadPageSettings: function() {
              this.load();
              let oCfg = this.getConfig();
              for(let oMsg of oCfg.msgs) {
                this.appendRFCode(oMsg.on,oMsg.msg,oMsg.type);
              }
              this.addSaveButton("RF433Content.savePageSettings");
            },
  
  savePageSettings: function() {
    let oCfg = this.getConfig();
    this.saveConfigValues(this.Area);
    oCfg.msgs = [];
    let oKnownCodes = this.getById("knownCodes");
    oKnownCodes.find("tr").each(function() {
      let oMsg = {}
      oMsg.on = $(this).data("msg");
      oMsg.msg = $(this).data("dev");
      oMsg.type = $(this).data("cmd");
      if(oMsg.on) oCfg.msgs.push(oMsg);
     });
     // oCfg.msgs = oMsg;
     setConfigChanged();
  },
  
  appendRFCode: function(nCode,nDevice,nCmd) {
            let oTableBody = this.getById("knownCodes");
            if(oTableBody.length > 0) {
              let strID = this.getFullId("msg_" + nCode);
              let strDevice = (nDevice  == ONAIR_CAMERA    ? "Camera" : "Microphone");
              let strCmd    = (nCmd     == ONAIR_DEVICE_ON ? "ON" : "OFF");
              // If row exists, change it, otherwise, create new...
              let oRow = $('#' + strID);
              if($("#"+strID).length > 0) { 
                oRow.html(""); 
              }
              else {
                oRow = $("<tr/>",{
                  id:strID,
                  class:"rf433-msgRow",
                }).appendTo(oTableBody);
              }
              oRow.data("msg",nCode);
              oRow.data("dev",nDevice);
              oRow.data("cmd",nCmd);
              $("<td/>",{ 
                title:"" + nCode
              }).html("0x" + nCode.toString(16)).appendTo(oRow);
              $("<td/>").html(strDevice).appendTo(oRow);
              $("<td/>").html(strCmd).appendTo(oRow);

              let oButtonCell = $("<td/>").appendTo(oRow);

              $("<button/>",{ id:"edit_" + strID,
                              style:"width:5em",
                              class:"btn btn-primary btn-sm pull-right rf433-enabled", 
                              onclick:'editRF433Message("' + strID + '")'
              }).html("Edit").appendTo(oButtonCell);

              $("<button/>",{ id:"del_" + strID,
                              style:"width:5em",
                              class:"btn btn-danger btn-sm pull-right rf433-enabled", 
                              onclick:'$("#' + strID + '").remove();'
                            }).html("Delete").appendTo(oButtonCell);
            }
          },
}
$.extend(RF433Content,PageHandler).setDataId("rf433");



var RF433Dialog = {
  SourceID: "panelSetRF433Code",  

  showDialog(strCurrentDataID) {
    this.loadDialog(this.SourceID,function(pDialog){
      if(strCurrentDataID) {
        pDialog.Area.find(".new-mode").attr("disabled",true);
        pDialog.getById("acceptButton").html("Change");
        let oRow = $("#" + strCurrentDataID);
        let nCode = oRow.data("msg");
        pDialog.setScanCode({ on:nCode });
        pDialog.getById("device").val(oRow.data("dev"));
        pDialog.getById("cmd").val(oRow.data("cmd"));  
      } else {
        pDialog.Area.find(".new-mode").attr("disabled",false);
        pDialog.getById("acceptButton").html("Add");
      }
    },this);
  },

  setScanBtnMode: function(strMode) {
    let oBtn = this.getById("scanButton");
    let strCurMode = oBtn.data("mode") ?? "norm";
    switch(strMode) {
      case "scan": oBtn.attr("disabled",true);
                  oBtn.html("...").attr("disabled".true).data("mode",strMode);
                  break;
      case "norm": if(strCurMode == "scan") oBtn.attr("disabled",false);
                  oBtn.html("Scan");
    }
  },

  startScanCode: function() {  
    this.setScanBtnMode("norm");
    this.getById("scanButton").html("...");
    sendWebsocket("{\"command\":\"scanRF433\"}");
  },

  setScanCode: function(oMsg) {
    let oNode = this.getById("scan_code");    
    oNode.val("0x" + Number(oMsg.on).toString(16));
    this.validateCode(oNode[0]);
    this.setScanBtnMode("norm");  
  },

  validateCode: function(oElement) {
    validateElement(oElement);
    let bInError = $("#dialogContent .inError").length > 0;
    this.getById("acceptButton").attr("disabled",bInError);
  },

  onEnabledChanged: function(oElement) {
    this.Area.find(".rf433-modified").prop("disabled",false);
  }
}
$.extend(RF433Dialog,DialogHandler).setDataId("rf433");

/*
function onRF433EnabledChanged(oElement) {
  $(".rf433-modified").prop("disabled",false);
}

function validateRF433Code(oElement) {
  validateElement(oElement);
  let bInError = $("#dialogContent .inError").length > 0;
  $("#rf433_acceptButton").attr("disabled",bInError);
}
*/
function editRF433Message(strID) {
  RF433Dialog.showDialog(strID);
  /*
  let oNode = $("#dialogContent");
  oNode.html($("#panelSetRF433Code").html()).promise().done(function(){
    adjustContentAttributes(oNode,"rf433");
    oNode.find(".new-mode").attr("disabled",true);
    $("#rf433_acceptButton").html("Change");
    let oRow = $("#" + strID);
    let nCode = oRow.data("msg");
    RF433.scanCodeReceived({ on:nCode });
    $("#rf433_device").val(oRow.data("dev"));
    $("#rf433_cmd").val(oRow.data("cmd"));    
    oNode.modal("show");
  });
  */
}

function addRF433Code() {
  let oCode = $("#rf433_scan_code").val();
  let oDevice = parseInt($("#rf433_device").val());
  let oCmd    = parseInt($("#rf433_cmd").val());
  let nCode = oCode.length > 2 && oCode[1] == 'x' ? parseInt(oCode,16) : parseInt(oCode);
  RF433Content.appendRFCode(nCode,oDevice,oCmd);
}

function newRF433Message() {
  let oNode = $("#dialogContent");
  oNode.html($("#panelSetRF433Code").html()).promise().done(function(){
    adjustContentAttributes(oNode,"rf433");
    oNode.find(".new-mode").attr("disabled",false);
    $("#rf433_acceptButton").html("Add");
    oNode.modal("show");
  });
}
/*
function scanRF433Code() {
  RF433.startScanCode();
}
  */

/*
function insertRF433Code(oMsg) {
  let oNode = $("#rf433_scan_code");
  logDebugEntry("- scan code received:");
  logDebugEntry(oMsg);
  
  oNode.val("0x" + Number(oMsg.on).toString(16));
  validateRF433Code(oNode[0]);

  $("#rf433_scanButton").html("Scan code");  
}
*/


/*
function deleteRF433Message(strID) {
  $("#" + strID).remove();
}
*/
/*
function appendRF433Row(nCode,nDevice,nCmd) {
  let oTableBody = $("#rf433_knownCodes");
  if(oTableBody.length > 0) {
    let strID = "rf433_msg_" + nCode;
    let strDevice = (nDevice  == ONAIR_CAMERA    ? "Camera" : "Microphone");
    let strCmd    = (nCmd     == ONAIR_DEVICE_ON ? "ON" : "OFF");
    // If row exists, change it, otherwise, create new...
    let oRow = $('#' + strID);
    if($("#"+strID).length > 0) { 
      oRow.html(""); 
    }
    else {
      oRow = $("<tr/>",{
        id:strID,
        class:"rf433-msgRow",
      }).appendTo(oTableBody);
    }
    oRow.data("msg",nCode);
    oRow.data("dev",nDevice);
    oRow.data("cmd",nCmd);
    $("<td/>",{ 
      title:"" + nCode
    }).html("0x" + nCode.toString(16)).appendTo(oRow);
    $("<td/>").html(strDevice).appendTo(oRow);
    $("<td/>").html(strCmd).appendTo(oRow);

    let oButtonCell = $("<td/>").appendTo(oRow);

    $("<button/>",{ id:"edit_" + strID,
                    style:"width:5em",
                    class:"btn btn-primary btn-sm pull-right rf433-enabled", 
                    onclick:'editRF433Message("' + strID + '")'
    }).html("Edit").appendTo(oButtonCell);

    $("<button/>",{ id:"del_" + strID,
                    style:"width:5em",
                    class:"btn btn-danger btn-sm pull-right rf433-enabled", 
                    onclick:'deleteRF433Message("' + strID + '")'
                  }).html("Delete").appendTo(oButtonCell);
  }
}
*/
function loadRF433Page() {
  RF433Content.loadPageSettings();
  /*
  $(".rf433-modified").prop("disabled",false);
  let oCfg = getConfigSection("rf433",true);
  setPropertiesOf(oCfg,"rf433");
  onRF433EnabledChanged(document.getElementById("rf433_enabled"));
  for(let oMsg of oCfg.msgs) {
    appendRF433Row(oMsg.on,oMsg.msg,oMsg.type);
  }
  addSaveButton(save433PageSettings);
  */
}
/*
function save433PageSettings() {
  let oCfg = getConfigSection("rf433",true);
  storeConfigValuesFrom(oCfg,["enabled"]);
  oCfg.msgs = [];
  $("#rf433_knownCodes tr").each(function() {
    let oMsg = {}
    oMsg.on = $(this).data("msg");
    oMsg.msg = $(this).data("dev");
    oMsg.type = $(this).data("cmd");
    if(oMsg.on) oCfg.msgs.push(oMsg);
   });
   // oCfg.msgs = oMsg;
   setConfigChanged();
}
*/
//#endregion

// #region OnAir Light Settings
// =========================================

$("#onair").on("click", (function() {
  switchToContentPage("#onaircontent");
  return false;
}));


function loadOnAirPage() {
  let oOnAirCfg = getConfigSection("onair",true);
  setConfigValuesTo(oOnAirCfg,["oncam","onmic"]);
  // oAirCfg.priority contains "cam" or "mic" - like the value in the GUI
  let oElement = $("input[name=\"onairpriority\"][value=\"" + oOnAirCfg.priority + "\"]");
  if(oElement) $(oElement).prop("checked",true);

  let oBrightCtrl = document.getElementById("onairbrightness");
  if(oBrightCtrl) {
    oBrightCtrl.innerHTML = "";
    for(let i = 1; i < 11; i++) {
      let oCtrl = document.createElement("option");
      oCtrl.textContent = (i * 10) + "%";
      oCtrl.value = (i * 10);
      oBrightCtrl.appendChild(oCtrl);
    }
    let nBrightVal = 100;
    if(Config.onair && Config.onair.brightness) {
      // To ensure aligned to full decimal numbers, i.E. 33 becomes 30
      nBrightVal = Math.round(Config.onair.brightness / 10) *10;
      if (nBrightVal < 1 || nBrightVal > 100) nBrightVal = 100;
    }
    oBrightCtrl.value = nBrightVal;
  }
}

function saveOnAirPage() {
  let oOnAirCfg = getConfigSection("onair",true);
  storeConfigValuesFrom(oOnAirCfg,["oncam","onmic"]);
  oOnAirCfg.brightness = $("#onairbrightness").val();
  oOnAirCfg.priority = document.querySelector("input[name=\"onairpriority\"]:checked").value;
  setConfigChanged();
}

//#endregion

// #region Backup content page
// =========================================

$("#backup").click(function() {
  switchToContentPage("#backupcontent");
  return false;
});

function loadBackupPage() {
  // Currently - nothing to do...
}

/**
 * Entry Point to start a backup
 */
function backupConfig() {
  // Request a config to be stored (partial encrypted)
  sendCommandMessage(GET_BACKUP_COMMAND);
}

function restoreBackup() {
  let oInput = document.getElementById("restoreSet");
  let oReader = new FileReader();
  logDebugEntry("Restore file selected...");
  if ("files" in oInput) {
    if (oInput.files.length === 0) {
      alert("You did not select file to restore!");
    } else {
      oReader.onload = function() {
        let oConfigData;
        try {
          oConfigData = JSON.parse(oReader.result);
          // Check if structure could be a config file...
          if (oConfigData.wifi && oConfigData.wifi.hasOwnProperty("ap_mode")) {
            if (confirm("File seems to be valid, do you wish to continue?")) {
              sendCommandMessage(RESTORE_BACKUP_COMMAND,"",oConfigData);
            }
          } else {
            alert("Not a valid backup file!");
            logDebugEntry("Invalid backup file!");
            logDebugEntry(oConfigData);
          }
        } catch (e) {
          alert("Invalid backup file!");
          return;
        }
        
      };
      oReader.readAsText(oInput.files[0]);
    }
  }
}



// #endregion


// #region Feedback/ticket content page
// =========================================

$("#feedback").click(function() {
  switchToContentPage("#feedbackcontent");
  return false;
});

function loadFeedbackPage() {
  $("#feedbackEmail").attr("onclick",
    "window.open('mailto:office@lsc-labs.de?subject=" + DeviceStatus.prog_name + " v" + DeviceStatus.prog_version + "')"
  )
  // Currently - nothing to do...
}

// #endregion


// #region System settings (Reboot / Factory Reset)
$("#system").click(function() {
  switchToContentPage("#systemcontent");
  return false;
});

function loadSystemPage() {
  // Currently - nothing to do...
}

function rebootDevice() {
  if(confirm('Are you sure you want to reboot this device ?\nAll unsaved changes will be lost,\nexisting clients will be disconnected..')) {
    sendCommandMessage(REBOOT_COMMAND);
  }
}

function resetToFactorySettings() {
  if(confirm('Are you sure you want to reset this device to factory settings?\nAll current settings will be deleted...')) {
    sendCommandMessage(FACTORY_RESET_COMMAND);
  }
}


// #endregion

// #region MQTT Broker page
// =========================================

$("#mqtt").click(function() {
    switchToContentPage("#mqttcontent");
    return false;
});

function loadMqttPage() {
    let oCfg = getConfigSection("mqtt",true);
    setConfigPropertiesOf("mqtt");
    updateMqttPageView();
    
}

function updateMqttPageView() {
  showElement("mqttenabledsettings",isMqttEnableSelected()); 
}

function isMqttEnableSelected() {
  return(document.getElementById("mqtt_enabled").checked)
}
/**
 * validate the user GUI entries to be stored.
 * @returns true if settings can be processed
 */
function validateMqttPage() {
  let isValid = true;
  // Access Point Mode - or STA without DHCP check...
  if (isMqttEnableSelected()) {
    // Access Point Mode - need a vlaid ipaddress and subnet
    isValid = checkElement("mqtt_port","Port number") &&
              checkElement("mqtt_syncrate","synchronisation rate");  
  } 
  return(isValid);
}

function saveMqttPage() {
  if(validateMqttPage()) {
    if(isMqttEnableSelected()) {
      storeElementValuesToConfigProperties("mqtt");
    } else {
      // to avoid loosing existing data only disable
      let oCfg = getConfigSection("mqtt",true);
      oCfg.enabled = "0";
    }
    setConfigChanged();
  }
    /*
    oConfig.mqtt.enabled = 0;
    if (parseInt($("input[name=\"mqttEnabled\"]:checked").val()) === 1) {
        oConfig.mqtt.enabled = 1;
    }
    else{
      oConfig.mqtt.enabled = 0;
    } 
    oConfig.mqtt.host      = document.getElementById("mqtthost").value;
    oConfig.mqtt.port      = parseInt(document.getElementById("mqttport").value);
    oConfig.mqtt.topic     = document.getElementById("mqtttopic").value;
    oConfig.mqtt.autotopic = document.getElementById("mqttautotopic").checked;
    oConfig.mqtt.user      = document.getElementById("mqttuser").value;
    oConfig.mqtt.pswd      = document.getElementById("mqttpwd").value;
    oConfig.mqtt.syncrate  = document.getElementById("syncrate").value;
    oConfig.mqtt.mqttlog   = 0;
    if (parseInt($("input[name=\"mqttlog\"]:checked").val()) === 1) {
      oConfig.mqtt.mqttlog = 1;
    }
    else{
      oConfig.mqtt.mqttlog = 0;
    } 
    oConfig.mqtt.mqttha = 0;
    if (parseInt($("input[name=\"mqttha\"]:checked").val()) === 1) {
      oConfig.mqtt.mqttha = 1;
    }
    else{
      oConfig.mqtt.mqttha = 0;
    } 
      */
    
}
//#endregion

//#region NTP Service
var utcSeconds;
$("#ntp").click(function() {
    getContent("#ntpcontent");
    return false;
});

function syncBrowserTime() {
    var d = new Date();
    var timestamp = Math.floor((d.getTime() / 1000));
    var datatosend = {};
    datatosend.command = "settime";
    datatosend.epoch = timestamp;
    sendWebsocket(JSON.stringify(datatosend));
    $("#ntp").click();
}

function deviceTime() {
    let t = new Date(utcSeconds * 1000); // milliseconds from epoch
    document.getElementById("device-time").innerHTML = t.toString();
  }
  
function load_ntp() {
    sendWebsocket("{\"command\":\"gettime\"}");
    document.getElementById("ntpserver").value = config.ntp.server;
    document.getElementById("intervals").value = config.ntp.interval;
    document.getElementById("DropDownTimezone").value = config.ntp.tzinfo;
    deviceTime();
}

function save_ntp() {
    config.ntp.server = document.getElementById("ntpserver").value;
    config.ntp.interval = parseInt(document.getElementById("intervals").value);
    config.ntp.tzinfo = document.getElementById("DropDownTimezone").value;
  
    setConfigChanged();
}
    
//#endregion

//#region Restart Progress Bar
/**
 * Show the restart / upload panel.
 * 
 */
function showRestartPanel(nTimeIntervall) {
  if(!nTimeIntervall) nTimeIntervall = 500;
  let strPageContent = $("#progresscontent").html();
  $("#progressPage").html(strPageContent).promise().done(function() {
    $("#mainPage").hide();
    $("#progressPage").show();
    let oProgressBar = document.getElementById("progressBar");
    $(oProgressBar).css("height", "80");
    $(oProgressBar).css("font-size", "large");
    let i = 0;
    let oTimer = setInterval(function() {
      let oProgressBar = document.getElementById("progressBar");
      oProgressBar.style.width = i + "%";
      oProgressBar.innerText = i + "%";
      // $("#progressBar").css("width", i + "%").attr("aria-valuenow", i).html(i + "%");
      i++;
      if (i === 101) {
        clearInterval(oTimer);
        if(Config && Config.hostname) {
          let a = document.createElement("a");
          a.href = "http://" + Config.hostanme + ".local";
          a.innerText = "Try to reconnect ESP";
          document.getElementById("reconnect").appendChild(a);
          document.getElementById("reconnect").style.display = "block";
        }
        oProgressBar.className = "progress-bar progress-bar-success";
        oProgressBar.innerText = "Completed";
      }
    }, nTimeIntervall);
  });
}
//#endregion

//#region Firmware update and Progress bar
$("#update").on("shown.bs.modal", function(e) {
  getLatestReleaseInfo();
});

function getLatestReleaseInfo() {
  // let strURL = "https://api.github.com/repos/esprfid/esp-rfid/releases/latest";
  let strURL = "https://api.github.com/repos/LSC-Labs/OnAir-Light/releases";
  $("#versionhead").text(DeviceStatus.prog_version);
  $.getJSON(strURL).done(function(tAllReleases) {
    let oRelease = tAllReleases[0];
    let oAsset = oRelease.assets[0];
    /*
    let nDownloadCount = 0;
    for (let i = 0; i < oRelease.assets.length; i++) {
      nDownloadCount += oRelease.assets[i].download_count;
    }
      */
    let oneHour = 60 * 60 * 1000;
    let oneDay = 24 * oneHour;
    let dateDiff = new Date() - new Date(oRelease.published_at);
    let timeAgo;
    if (dateDiff < oneDay) {
      timeAgo = (dateDiff / oneHour).toFixed(1) + " hours ago";
    } else {
      timeAgo = (dateDiff / oneDay).toFixed(1) + " days ago";
    }
    let strReleaseDateTime = oRelease.published_at ? oRelease.published_at.substring(0,10) : "-";

    let strReleaseInfo = '"' + oRelease.tag_name + ' (' + oRelease.name + ')" was updated on ' + strReleaseDateTime + " - ("+ timeAgo + ").";
    // and downloaded " + nDownloadCount.toLocaleString() + " times.";
    $("#downloadupdate").attr("href", oAsset.browser_download_url);
    $("#releasehead").text(strReleaseInfo);
    $("#releasebody").text(oRelease.body);
    $("#releaseinfo").fadeIn("slow");
  }).error(function() {
    $("#onlineupdate").html("<h5>Couldn't get release info. Are you connected to the Internet?</h5>");
  });
}




function allowUploadFirmware() {
  $("#upbtn").prop("disabled", false);
}

function uploadFirmware() {
  let formData = new FormData();
  formData.append("bin", $("#binform")[0].files[0]);
  showRestartPanel();
//   inProgress("upload");
  $.ajax({
    url: "/update",
    type: "POST",
    data: formData,
    processData: false,
    contentType: false
  });
}
//#endregion

/**
 * Main entry point
 */
function start() {
  /*
    let maincontent = document.createElement("div");
    maincontent.id = "mastercontent";
    maincontent.style.display = "none";
    document.body.appendChild(maincontent);
    */
    $("#signin").on("shown.bs.modal", function() {
      $("#password").focus().select();
    });
    $("#mastercontent").load("main.htm", function(responseTxt, statusTxt, xhr) {
        if (statusTxt === "success") {
          switchToContentPage("#statuscontent");
          $("#signin").modal({
              backdrop: "static",
              keyboard: false
          });
          $("[data-toggle=\"popover\"]").popover({
              container: "body"
          });
        }
    });
    
}
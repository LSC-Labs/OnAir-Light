/**
 * This file is handling the index page
 * 
 * Expectation is a menu structure with <li> elements (also subemenus)
 * 
 * An Content container with an id of "mastercontent" 
 * 
 * 
 */




// #region Content Page switching
// =========================================

class CContentManager {

    constructor(strPrefix) {
      this.Prefix = strPrefix;
    }
  
    /**
     * get the prefix to be set by config and id infos.
     * If set in the constructer, it is the Prefix in lowercase with trailing "_"
     * If not set, result is ""
     * @returns the prefix used on this content 
     */
    getPrefixOfID() {
      return(this.Prefix ? this.Prefix.toLowerCase() + "_" : "");
    }
  
    /**
     * Try to find the load content function by default
     * Set the Prefix like "Network" and the result will search "loadNetworkPage"
     * If the function does not exist, it will be undefined.
     * @returns 
     */
    getDefaultLoadFunction() {
      let strName = "loadData";
      if(this.Prefix) strName = "load" + strPrefix + "Page";
      let pLoadFunction = Windows[strName];
      return(typeof pLoadFunction === 'function' ? pLoadFunction : undefined);
    }
  
    /**
     * Returns the target as jQuery object..
     * Either it is a string => search the element by id defined in string
     * Or it is already a jquery object -> return the object,
     * Otherwise it is a HTMLElement -> convert to jQuery object
     * undefined - if nothing could be resolved.
     * @param {*} oTargetSpec 
     */
    getJQueryObject(oTargetSpec) {
      let oTarget;
      if(oTargetSpec) {
        // jQuery object ?
        if(oTargetSpec instanceof jQuery) oTarget = oTargetSpec;
        else if(oTargetSpec.constructor) {
          // String or HTML*element
          if(oTargetSpec.constructor.name == 'String') oTarget = $("#" + oTargetSpec);
          else oTarget = $(oTargetSpec);
        }
      }
    }
  
    /**
     * Set the correct page attributes.
     * To avoid duplicate id's on DOM, the templates should use "data-id" to avoid conflicts
     * data-id    - will become id, if not already set
     * data-cfg   - will become id, if not already set
     * data-val   - will implement onchange="validateElement(this)" if onchange is not in place
     * @param {*} oTargetSpec id, jquery or document element
     */
    adjustAttributesOnTarget(oTargetSpec) {
      let oTarget = this.getJQueryObject(oTargetSpec);
      if(oTarget && oTarget.length > 0) {
        // insert validate function if not already in place...
        oTarget.find("[data-val]").each(function(nIdx) {
          if(!$(this).attr(onkeyup)) $(this).attr("onkeyup","validateElement(this)");
        });
        // adjust the id's and settings...
        logDebugEntry(" - switching content id's of page : " + strContent);
        oTarget.find("[data-id]").each(function() {
          if(!$(this).attr("id")) {
            $(this).attr("id", strIDPrefix + "_" + $(this).attr("data-id"));
          }
        });
        oTarget.find("[data-cfg]").each(function() {
          if(!$(this).attr("id")) {
            $(this).attr("id", strContent + "_" + $(this).attr("data-cfg"));
          }
        });
      }
    }
  
    /**
     * load html content to an element and adjust the properties of the elements
     * all elements with "data-val" will enriched with validateElement(this), if not already in place
     * data-id will become the id of the element (with prefixing) if not already in place
     * data-cfg will become the id of the element (with prefixing) if not already in place
     * @param {*} strHTML 
     * @param {*} oTargetSpec "id", document.element or jQuery element
     * @param {*} funcLoadContent function load values into the page, autogen ("load" + strPrefix + "Page") 
     * @param {*} bDisplayContent show the content when loaded...
    */
    createTargetContent(strHTML, oTargetSpec, bDisplayContent, funcLoadContent) {
      let oTarget = this.getJQueryObject(oTargetSpec);
      if(oTarget && oTarget.length > 0) {
        oTarget.html(strHTML).promise().done(function() {
          this.adjustAttributesOnTarget(oTarget);
          // Call the load function if defined and available
          // either the user has specified explicit, or try to find by name...
          let pLoadFunction = funcLoadContent;
          if(!pLoadFunction) pLoadFunction = this.getDefaultLoadFunction();
          if(pLoadFunction) pLoadFunction(oTarget,this);
          // Adjust the popup functions
          $("[data-toggle=\"popover\"]").popover({
            container: "body"
          });
          if(bDisplayContent) $(this).hide().fadeIn();
        });
      } else{
        logDebugEntry("[E] Element not found : " + strElementID);
      } 
      
    };
  
    loadAndSwitchTo(oSourceSpec, oTargetSpec, bDisplayContent, funcLoadContent) {
      if(!oTargetSpec)      oTargetSpec = "ajaxcontent";
      if(!bDisplayContent)  bDisplayContent = true;
      let oSource = this.getJQueryObject(oSourceSpec);
      $("#dismiss").click();
      $(".overlay").fadeOut().promise().done(function() {
        createTargetContent(oSource.html(),oTargetSpec,bDisplayContent,funcLoadContent);
      })
    }
  
  };
  
  ContentMgr = new CContentManager();
  
function runTest() {
    let oBuilder = new CModuleCreator(oSampleModule);
    oBuilder.insertMenuEntries();
    activatePage("rf433");

}

var oSampleModule = {
    type: "Settings",
    id:  "rf433",
    menus: [{ type:"Settings", txt: "Remote controller", icon: "repeat", on: { click: "@" } }],
    pages: [{ legend:"RF 433 Settings", 
              title:"Setup of a standard remote controller (433 Mhz)",        
            fgroups: [
                { 
                    col_label: { txt:"Remote control", info:"Enable or disable the remote control function (433 Mhz)"},
                    col_check: { id:"enabled", on: { click:"@" }, enabled: true, label: "Enable" }
                },
                { 
                    hidden: true,
                    col_label: { txt:"Known remotes", info:"List of registered remote contorls"},
                    col_table: { id:"known_remotes", data: { onload:"@", onsave:"@" }, header:["code","device","action"]}
                }
            ]
        
            
    }]
}

class CModuleCreator {
    constructor(oModule, oPageDef) {
        this.Module  = oModule;
        this.PageDef = oPageDef ?? oModule.pages[0];
        
        this.DEFAULT_LABEL_CLASS = "col-xs-5",
        this.DEFAULT_INPUT_CLASS = "col-xs-7 col-md-5"
    }

    ucFirst(str) {
        return(str[0].toUpperCase() + str.slice(1))
    }

    insertMenuEntry(oMenuDef) {
        let strParentName = "Nav" + (oMenuDef.type ?? "Settings") + "Submenu";
        let oParent = $("#" + strParentName);
        if(oParent) {
            let strID = oMenuDef.id ?? this.Module.id ?? "none";
            let oMenuEntry = $("<li/>",{ id:strParentName + "_" + strID}).appendTo(oParent);
            let oAnchorLink = $("<a/>",{ href:"#", id:strID }).appendTo(oMenuEntry);
            for(let strOnName in oMenuDef.on) {
                let strTarget = oMenuDef.on[strOnName];
                // if(strTarget == '@') strTarget = "on_" + strID + "_" + strOnName;
                if(!strTarget || strTarget == '@') strTarget = 'activatePage("' + strID + '",this)';
                $(oAnchorLink).attr("on" + strOnName,strTarget);
            }
            this.createGlyphIcon(oMenuDef.icon).appendTo(oAnchorLink);
            $("<span>").html(oMenuDef.txt).appendTo(oAnchorLink);
        }
    }

    insertMenuEntries() {
        if(this.Module) {
            for(let oMenu of this.Module.menus) {
                this.insertMenuEntry(oMenu);
            }
        }
    }

    /**
     * add config name and id, if in place...
     * cfg will be "cfg", or "id"
     * @param {*} oElement 
     * @param {*} oDef 
     */
    addDefaultProperties(oElement, oDef) {
        let strModuleID = this.Module.id;
        let strCfg = oDef.cfg ?? oDef.id;
        if(strCfg)  oElement.attr("data-cfg",strCfg);
        if(oDef.id) oElement.attr("data-id",oDef.id);
        if(oDef.enabled) oElement.enabled = oDef.enabled;
        for(let strOnName in oDef.on) {
            let strAttr = "on" + strOnName;
            let strCommand = oDef.on[strOnName];
            if(strCommand == '@') strCommand = 'funcOn_' + strModuleID + "_"  + strCfg + "_" + strOnName + '(this)';
            oElement.attr(strAttr,strCommand);
        }
        for(let strDataName in oDef.data) {
            oElement.attr("data-" +strDataName,oDef.data[strDataName]);
        }
        
    }
    
    createGlyphIcon(strGlyphName,strPopupText) {
        let oGlyph = $("<i/>",{
            class: "glyphicon glyphicon-" + strGlyphName ?? "",
        });
        if(strPopupText) {
            oGlyph.attr("style","margin-left: 10px")
                  .attr("aria-hidden","true")
                  .attr("data-toggle","popover")
                  .attr("data-trigger","hover")
                  .attr("data-placement","right")
                  .attr("data-content",strPopupText)
        }
        return(oGlyph)
    }

    createColLabel(oDef) {
        let oElement = $("<label/>", { class: DEFAULT_LABEL_CLASS });
        $("<span/>").html(oDef.txt).appendTo(oElement);
        this.createGlyphIcon("info-sign",oDef.info).appendTo(oElement);
        return(oElement);
    }

    createColCheckBox(oDef) {
        let oSpan = $("<span/>",{ class: this.DEFAULT_INPUT_CLASS});
        let oLabel = $("<label/>",{ class:"ElementLabel"}).appendTo(oSpan);
        let oInput = $("<input>",{ "type": "checkbox", }).appendTo(oLabel);
        this.addDefaultProperties(oInput,oDef);
        if(oDef.label) {
            let oTxt = $("<span/>",{class:"ElementLabelText"}).appendTo(oLabel);
            oTxt.html(" " + oDef.label);
        }
        return(oSpan);
    }

    createColTable(oDef) {
        let oTable = $("<table/>",{ class: this.DEFAULT_INPUT_CLASS});
        this.addDefaultProperties(oTable,oDef);
        let oTH = $("<thead/>").appendTo(oTable);
        for(let strTitle of oDef.header) {
            $("<th/>").html(strTitle).appendTo(oTH);
        }
        $("<tbody/>",{ "data-id":oDef.id + "_data"}).appendTo(oTable);
        return(oTable);
    }

    createFormGroups(oDef) {
        let oResult = $("<div/>",{class:"form-group-rows"});
        for(let oFormGroupDef of oDef) {
            let oFormElement = $("<div/>",{class:"row form-group"}).appendTo(oResult);
            this.appendChildElements(oFormElement,oFormGroupDef);
        }
        return(oResult);
    }
        
    createElement(strName, oObj) {
        let oResult;
        if(strName && oObj) {
            // First simple objects...
            switch(strName) {
                case "legend"   : oResult = $("<legend/>").html(oObj); break;
                case "label"    : oResult = $("<label/>").html(oObj); break;
                case "title"    : oResult = $("<h6/>",{ class:"text-muted"}).html(oObj);
                case "ce"       : for(let oSubDef of oObj) this.appendChildElements(oResult,oSubDef); break;
                case "fgroups"  : oResult = this.createFormGroups(oObj); break;
                case "col_label": oResult = this.createColLabel(oObj); break;
                case "col_check": oResult = this.createColCheckBox(oObj); break;
                case "col_table": oResult = this.createColTable(oObj); break;
            }        
        }
        return(oResult);
    }

    appendChildElements(oNode, oDef) {
        if(oNode && oDef) {
            for(let strProp in oDef) {
                let oElement = this.createElement(strProp,oDef[strProp]);
                if(oElement) oElement.appendTo(oNode);
            }
        }
    }

    createPage(oPageDef) {
        let oPage = $("<span/>");
        if(!oPageDef && this.Module.pages) oPageDef = this.Module.pages[0];
        if(oPageDef) {
            let strPageID = (oPageDef.id ? oPageDef.id : this.Module.id) + "_PageContent";
            oPage = $("<div/>",{ id: strPageID });
            this.appendChildElements(oPage,oPageDef);
        }
        return(oPage);
    }
}


var ModuleDefs = {
    "modules": [
        oSampleModule,
    ]
}

function activatePage(strPageID,oSenderElement) {
    console.log("activatePage(" + strPageID + "," + (oSenderElement ? oSenderElement.id : "?") + ")");
    // Dismiss the main menu... (if shown...)
    $("#dismiss").click();
    // main overlay layer fade out... and action !
    $(".overlay").fadeOut().promise().done(function() {
        // If the page is static defined, use this content...
        let strPageContent = $(strPageID).html();
        if(!strPageContent || strPageContent.length() < 1) {
            // try to generate the page by JavaScript defintiion
            for(let oModule of ModuleDefs.modules) {
                for(let oPageDef of oModule.pages) {
                    let strID = oPageDef.id ?? oModule.id;
                    if(strID == strPageID) {
                        let oBuilder = new CModuleCreator(oModule,oPageDef);
                        let oPage = oBuilder.createPage(oPageDef);
                        let oPageContent = oPage.html();
                        $("#ajaxcontent").html(oPageContent).promise().done(function() {
                            // Activate the popups !
                            $("#ajaxcontent [data-toggle=\"popover\"]").popover({
                                container: "body"
                            });
                            console.log("Page generated :");
                            console.log(document.getElementById("ajaxcontent"));
                        });
                    }
                }
            }
        }
    });
}

const DEFAULT_LABEL_CLASS = "col-xs-5";
const DEFAULT_INPUT_CLASS = "col-xs-7 col-md-5";

function getPropertyValueOrDefault(strPropertyName, tElements, strDefault) {
    let strResult = strDefault;
    if(tElements) {
        for(let oElement of tElements) {
            if(oElement[strPropertyName]) {
                strResult = oElement[strPropertyName];
                break;
            }
        }
    }
    return(strResult);
}


function appendFormGroupDef(oPageDef, oFormGroupDef) {
    if(oFormGroupDef) {
        let oFormElement = document.createElement("div");
        oFormElement.className = "row form-group";
        if(oFormGroupDef.label) {
            let oLabel = document.createElement("label");
            oLabel.className = getPropertyValueOrDefault("labelclass",{oFormGroupDef,oPageDef}, DEFAULT_LABEL_CLASS);
            oFormElement.appendChild(oLabel);
        }

    }
}

function createLandingPageTemplates(oPageDef) {
    if(oPageDef) {
        for(let oPage of oPageDef.pages) {
            let oParent = document.getElementById("mastercontent");
            if(oParent) {
                let strPageID = (oPage.id ? oPage.id : oPageDef.id) + "_PageContent";
                let oPageElement = document.createElement("div");
                oPageElement.id = strPageID;
                if(oPage.legend) { 
                    let oLegend = document.createElement("legend");
                    oLegend.innerHTML = oPage.legend;
                    oPageElement.appendChild(oLegend);
                }
                if(oPage.txt) {
                    let oTxt = document.createElement("h6");
                    oTxt.className = "text-muted";
                    oTxt.innerHTML = oPage.txt;
                    oPageElement.appendChild(oTxt);
                }
                for(oFormGroup of oPage.form_groups) {
                    let oFormElement = document.createElement("div");
                    oFormElement.className = "row form-group";
                    if(oFormGroup.label) {

                    }
                }
            }

        }
    }
}

function createLabel(strText,strHelpText) {
    let oElement = document.createElement("label");
    oElement.className = DEFAULT_LABEL_CLASS;
    oElement.innerHTML = strText + createGlyphIcon("info-sign",strHelpText).html();
   /*
    <label class="col-xs-5">MQTT<i style="margin-left: 10px;" class="glyphicon glyphicon-info-sign" aria-hidden="true" data-toggle="popover" data-trigger="hover" data-placement="right" data-content="Please choose if you want to enable MQTT"></i></label>
    */

}




/**
 * create Page entries as defined
 * 
 * {
 *    menu: {
 *      parent: "Settings",     // Settings or Admin
 *      id: "id",
 *      text: "text to be displayed",
 *      glyph: "name of glyph icon like 'signal' or 'link'"
 *      on: {
 *        "click": "function to be calles"
 *      }
 *    } 
 * }
 * @param {*} oPageDef 
 */
function createContentPageOn(oElement,oPageDef) {
    if(oElement && oPageDef) {
        let strPageID = (oPageDef.id ? oPageDef.id : oElement.id) + "_PageContent";
        let oPageElement = document.createElement("div");
        oPageElement.id = strPageID;
        if(oPageDef.legend) { 
            let oLegend = document.createElement("legend");
            oLegend.innerHTML = oPageDef.legend;
            oPageElement.appendChild(oLegend);
        }
        if(oPageDef.txt) {
            let oTxt = document.createElement("h6");
            oTxt.className = "text-muted";
            oTxt.innerHTML = oPageDef.txt;
            oPageElement.appendChild(oTxt);
        }
        for(oFormGroup of oPageDef.form_groups) {
            let oFormElement = document.createElement("div");
            oFormElement.className = "row form-group";
            if(oFormGroup.label) {
                oFormElement.appendChild(createLabel(oPageDef.label,oPageDef.info));
            }

        }
        // Load the dynamic content...
        $(oElement).html(oPageContent.html());

    }
}

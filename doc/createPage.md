## Create a Page
A page is a part inside the web framework that shows or describes parts of your module

The frontend consits of a menu bar, a view/edit container and supports multi language, you have to write these parts if needed and join them into your build progress.

First start by creating a folder for your page, recommended inside the folder `src/web/pages/<PageID>`.

Inside this folder create the following components:

| Part |  |Function|
|---- | - |-- |
| page.html | optional | HTML code of the page |
| page.js   | optional | Javascript code for your page |
| page.css  | optional | Stylesheet information for your page
| i18n/en.js | optional | The default language file (english) |
| i18n/de.js | optional | The german language file if needed |
| i18n/<xx>.js | optional | Additional language files, wher <xx> is the language code used by the browser.

As you can see, all components are optional and not required, but if they are in place, they will be joined into the final code by the build-Pages npm job.

The following parts will explain how to write the parts and what are the major parts you have to think about.

### The build-Pages npm job
The build pages job uses the `pages.json` file and builds the final source code to be used by your application. Do NOT change these files as the job will build them and you will lose your modifications.

| file | function |
|----- |--------  |
| `src/web/_pages.html`| The job searches the page.html and join it into this file. If this file is mal formed, your whole page container will be inconsistent and you will see garbage. |
| `src/web/js/_pages.js`| The job searches ths page.js and join it into this file.
| `src/web/i18n/<xx>.json` | The job scans the language files of the pages and json them into this folder. Default language is `en` and if you want to use this feature it is recommended to have at least this file (en.json) in place. Keep in mind, this file will NOT be deleted before creation and if you decide to change the structure of a key inside (from a key = value to a key = object), you have to correct it inside the final file or you will receive an error.


### page.html
When creating the html code, you have to ensure to have it well formed with a unique id, as this id may only be in place one time in a correct html document. In detail, start by creating this file inside your page folder.

````
<div id="OnAirPage" 
     data-menu='{
                "name":"Settings|OnAir" 
                "title":"On Air",
                "icon":"Envelope"
                }'>
    <legend data-i18n="OnAir.legend"></legend>
    <!-- to be continued -->
</div>
```` 

This snippet shows some basics:
- id="OnAirPage" is a unique id for your page that reflects to the key `onair`. Per default this key is used to join to your configuration settings.



- All pages have to be defined in the file `pages.js` in the section `usePages`. @see build-Pages npm job.  
Insert an entry that points to your page into this section.
- 
- All page.js files of your application are joined into the file `src/web/_pages.js`


- The html has to be well formed. As all pages are loaded into a container




As an example, let's asume you have a module in place, that is able to read the temperature via a sensor. For this reason you need to adjust the raw data of this sensor to reflect the correct temperature.

You defined a unique id that your module is using called "sensorOD" for "sensor outdoor".


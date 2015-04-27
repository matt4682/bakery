# bakery
Automates repetitive build steps from a JSON-based instruction set

#### Usage
-----------
`bakery <recipe file>`

#### Sample Recipe
------------------
    {
      "ingredients": {
        "html" : "*.php",
        "php"  : "include",
        "css"  : "css",
        "js"   : "js"
      },
    
      "utensils": {
        "htmlminify" : "utensils/htmlcompressor-1.5.3.jar -o build/%i --compress-js --compress-css %i",
        "phpcompile" : "C:/xampp/php/php.exe %i > build/%n.html",
        "cssminify"  : "utensils/yuicompressor-1.5.3.jar -o build/css/%i %i",
        "jsminify"   : "utensils/yuicompressor-1.5.3.jar -o build/js/%i %i"
      },
       
      "instructions": [
        {"run" : "phpcompile", "on":"php"},
        {"run" : "phpcompile", "on":"html"},
        {"run" : "cssminify",  "on":"css"},
        {"run" : "jsminify",   "on":"js"}
      ]
    }
#### Advanced Guide
-------------------

##### Ingredients
Ingredients can be:
- a glob expression - `*.html`  
- a folder string - `js`  

##### Utensils
A command line argument that supports a few generic replacements
- %i - returns the selected ingredient - `index.html`
- %n - returns the selected filename - `index`

##### Instructions
A collection of JSON objects with two keys
- `run`: A utensil key
- `on`: An ingredient key

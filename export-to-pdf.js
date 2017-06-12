"use strict";

var markdownpdf = require("markdown-pdf")
  , fs = require("fs")


var path = "./";
 
fs.readdir(path, function(err, items) {
	var tpFolder = items.filter( (i) => { return i.indexOf("TP") != -1; } );

	// For TP folder
    for (var i=0; i < tpFolder.length; i++) {
    	var subPath = path + tpFolder[i] + "/report/";

    	let stats = fs.statSync(subPath);

    	// If there's a report
		if (stats.isDirectory()) {
			var items = fs.readdirSync(subPath);

			var mdFiles = items.filter( (i) => { return i.indexOf(".md") != -1; } );
			console.log("Found those files in \"" + subPath + "\" :");
			console.log("\t" + mdFiles.join("\n\t"));

			// For each md file
			for (var j=0; j < mdFiles.length; j++) {
				let mdFile = subPath + mdFiles[j];

				markdownpdf({ paperFormat: "A4" }).from(mdFile).to(mdFile.replace(".md", ".pdf"), function () {
					console.log(mdFile + " exported to pdf");
				}.bind(mdFile))
			}
		}
    }
});

/*
markdownpdf().from("/path/to/document.md").to("/path/to/document.pdf", function () {
  console.log("Done")
})

*/

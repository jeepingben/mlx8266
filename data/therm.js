var xhr = new XMLHttpRequest();  
var xhrb = new XMLHttpRequest();  
var min;
var max;
var units;
var unitString;
var temperature;
var alarm = 'Off';
var alarmVal;
var alertDisplayed=false;
var chart;
var tempSeries = new TimeSeries();
var chartCanvas;
      var theDiv;
	   var mmDiv;
	   //<link href="therm.css" rel="stylesheet"> 
  window.onload = function () {
  	     
  	     
  	     chartCanvas=document.getElementById("chart");

createTimeline();

unitsC.onclick = tempChange;
unitsF.onclick = tempChange;

alarmOff.onclick = alarmChange;
alarmHigh.onclick = alarmChange;
alarmLow.onclick = alarmChange;

graphEn.onclick = graphEnabled;

dismissDiv.onclick = dismissAlarm;
theDiv = document.getElementById('temperatureDiv');
mmDiv = document.getElementById('minMaxTempDiv');
  	     var audio = new Audio('alarm.ogg');
        'use strict';
        document.getElementById('resetMinMax').onclick=resetMinMax;
        var millisecondsBeforeRefresh = 1000; //Adjust time here
        //Fetch temperature
        var tempSlider = document.getElementById('tempSlider');
		  var tempCurDiv = document.getElementById("tempCurDiv");
tempSlider.oninput = function() {
	 	  alarmVal = this.value;
        tempCurDiv.innerHTML = this.value + "°" + unitString;
};
tempChange();
        xhr.onreadystatechange = function () {
        // process the server response
        if (xhr.readyState === 4) {
            // everything is good, the response is received
            if (xhr.status === 200 || xhr.status===0) {
                // perfect!
                var temps = JSON.parse(xhr.responseText);
                
                if (units === 'F') {
                temperature = temps.tObjF;
             } else {
             	temperature = temps.tObjC;
             }
               chart.resize();
             	tempSeries.append(new Date().getTime(), temperature);
             	
                theDiv.innerHTML = temperature + unitString;
                if ((alarm === 'Low' && temperature < alarmVal) ||
                    (alarm === 'High' && temperature > alarmVal)) {
                    	dismissDiv.style.display = 'block';
                	audio.play();
                }
                if (!max || temperature > max)
                {
                	max = temperature;
                }
                if (!min || temperature < min)
                {
                	min = temperature;
                }
                
                mmDiv.innerHTML = "Min: " + min + unitString + '  Max: ' + max + unitString;
                
            } else {
                // there was a problem with the request,
                // for example the response may contain a 404 (Not Found)
                // or 500 (Internal Server Error) response code
              //  console.log(xhr.status);
            } 
         } else {
             	
         }
     };
     xhrb.onreadystatechange = function () {
        // process the server response
        if (xhrb.readyState === 4) {
            // everything is good, the response is received
            if (xhrb.status === 200 || xhrb.status===0) {
                // perfect!
                console.log(xhrb.responseText)
                var batt = JSON.parse(xhrb.responseText);
                battDiv = document.getElementById('battDiv');
                
                battDiv.innerHTML = "Battery: " + batt.batt + "%";
                
                
            } else {
                // there was a problem with the request,
                // for example the response may contain a 404 (Not Found)
                // or 500 (Internal Server Error) response code
                //console.log(xhrb.status);
            } 
         } else {
         }
     };
        // Get the initial value, then fetch every second
        getTemperatureData();
        window.setInterval(getTemperatureData, millisecondsBeforeRefresh)
	    
	    
	    // Get the initial value, then fetch every few seconds
	    getBatteryLevel();
        window.setInterval(getBatteryLevel, millisecondsBeforeRefresh * 9)
	    };
    
    function getTemperatureData() {
        	try{
        		
    			xhr.open('GET', '/temp.json', true);
    			
            xhr.send();
				}
				catch(err){
				    console.log(err);
				}
        
	        }
 function getBatteryLevel() {
 		try{
        		
    			xhrb.open('GET', '/batt.json', true);
    			
            xhrb.send();
				}
				catch(err){
				    console.log(err);
				}
        
	        }
	     

 function tempChange() {

    if (unitsF.checked) {
        units='F';
        
    } else {
        units='C';
    }
	resetMinMax();
	tempSeries.clear();
	unitString = "˚" + units;
	tempSlider.oninput();
}

function dismissAlarm() {
	alarmOff.checked = (true);
	alarm = 'Off';
	dismissDiv.style.display = 'none';
	}

function alarmChange() {
	if (alarmHigh.checked) {
		alarm='High';
	} else if (alarmLow.checked) {
		alarm='Low';
	} else {
		alarm='Off';
	}
}

function graphEnabled() {
	if (graphEn.checked)
	{
		chart.start();
	} else {
		
		tempSeries.clear();
		chart.stop();
	}
}

function createTimeline() {
  chart = new SmoothieChart({millisPerPixel:2000,maxDataSetLength:2,grid:{millisPerLine:60000,sharpLines: true}});
  chart.addTimeSeries(tempSeries, { strokeStyle: 'rgba(255, 0, 0, 0.5)', fillStyle: 'rgba(50, 50, 50, 0.0)', lineWidth: 4 });
  chart.streamTo(chartCanvas, 5);
  resize(chartCanvas);
}
      
function resetMinMax()
{
	min='';
	max='';
}

function resize(canvas) {
  // Lookup the size the browser is displaying the canvas.
  var displayWidth  = tempSlider.clientWidth;
  var displayHeight = canvas.clientHeight;
  var w = Math.min(canvas.clientWidth, window.innerWidth || 0)
var h = Math.max(canvas.clientHeight, canvas.innerHeight || 0)
  console.log(canvas.clientWidth +","+canvas.clientHeight+","+canvas.width+","+canvas.height );
  // Check if the canvas is not the same size.
  if (canvas.width  != w ||
      canvas.height != h) {

    // Make the canvas the same size
    canvas.width  = w;
    canvas.height = h;
  }
}
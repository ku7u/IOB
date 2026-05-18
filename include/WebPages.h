#pragma once
#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en-US">
    <head>
    <link rel="stylesheet" href="stylesheet.css">
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>OLS Main Menu</title>
    </head>
    <body>
    <div class="sidebar">
    <a class="active" href="index.html">Home</a>
    <a href="apselect.html">Access Point</a>
    <a href="network.html">Network</a>
    <a href="locoparms.html">Locomotive</a>
        <a href="functions.html">Functions</a>
        <a href="calibration.html">Calibration</a>
        <a href="/update">Update</a>
        </div>
        <div class="content">
        <h2>Onboard Loco Simulator Configuration</h2>
        <table>
        <tr><td>OLS Firmware Version - </td><td> %version%</td></tr>
        </table>
        <p>The OLS device requires information about the network, the locomotive prototype parameters,
        and the function definitions in the model. Existing values are displayed.</p>
        <p>
        The values entered here will be stored aboard the loco and need not be entered again unless
        changes are required.
        Please note that certain changes will require the software to be rebooted.</p>
        <p>
        The OLS application itself can be updated over the wireless connection. If a new version has been supplied, use
        the <i>Update</i> menu item.</p>
    </div>
    </body>
    </html>
    )rawliteral";

// --- Network Page ---
const char network_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en-US">
    <head>
    <link rel="stylesheet" href="stylesheet.css">
    <title>OLS Network Parameters</title>
    <meta charset="UTF-8">
    <meta name="author" content="George Hofmann">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    </head>
    <body>
    <div class="sidebar">
        <a href="index.html">Home</a>
        <a class="active" href="network.html">Network</a>
        <a href="locoparms.html">Locomotive</a>
        <a href="functions.html">Functions</a>
        <a href="calibration.html">Calibration</a>
        <a href="/update">Update</a>
        </div>
        <div class="content">
        <h2>Network Parameters</h2>
        <table>
        <tr><td>SSID:</td><td> %SSID%</td></tr>
        <tr><td>Signal strength: </td><td> %RSSI%</td></tr>
        <tr><td>MAC address: </td><td>%MAC%</td></tr>
        <tr><td>IP address:</td><td> %IP%</td></tr>
        <tr><td>mDNS URL: </td><td>%MDNS%</td></tr>
        </table>
        <hr><br>
        <h2>AP Selection</h2>
        <p>To select a WiFi access point click the button below.</p>
        <a href="apselect.html"><button>Select AP</button></a>
        </div>
        </body>
        </html>
)rawliteral";

const char locoparms_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en-US">
    
    <head>
    <link rel="stylesheet" href="stylesheet.css">
    <title>OLS Loco Parameters</title>
    <meta charset="UTF-8">
    <meta name="author" content="George Hofmann">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    </head>
    
    <body>
    
    <div class="sidebar">
    <a href="index.html">Home</a>
    <a href="network.html">Network</a>
    <a class="active" href="locoparms.html">Locomotive</a>
    <a href="functions.html">Functions</a>
        <a href="calibration.html">Calibration</a>
        <a href="update/index.html">Update</a>
        </div>

    <div class="content">
    <h2>Locomotive Parameters</h2>
    
    <table>
    <tr>
    <td>Odometer:</td>
                <td> %ODOMETER%</td>
                <td> miles</td>
                </tr>
                </table>
                <hr>
                <br>
                <p>Assign all parameters associated with this locomotive.
            If unknown, retain the default.</p>
            <form action="/get">
            <input type="hidden" name="locoparmsParm" value="0">
            <table>

                <tr>
                    <td><label for="dccaddress">DCC Address</label></td>
                    <td><input type="number" id="dccaddress" name="dccaddress" value="%DCCADDRESS%" min="1" max="9999"
                            style=text-align:center></td>
                            </tr>
                <tr>
                <td><label for="locoid">Loco ID</label></td>
                <td><input type="text" id="locoid" name="locoid" value="%LOCOID%" style=text-align:center></td>
                </tr>
                <tr>
                    <td><label for="locotype">Loco Type</label></td>
                    <td><input type="text" id="locotype" name="locotype" value="%LOCOTYPE%" style=text-align:center></td>
                    </tr>
                <tr>
                <td><label for="horsepower">Horsepower</label></td>
                <td><input type="number" id="horsepower" name="horsepower" value="%HORSEPOWER%" min="100" max="7000"
                  style=text-align:center></td>
                </tr>
                <tr>
                <td><label for="weight">Weight (lbs)</label></td>
                <td><input type="number" id="weight" name="weight" value="%WEIGHT%" min="50000" max="500000"
                  style=text-align:center></td>
                </tr>
                <tr>
                <td><label for="tractiveeffort">Tractive Effort (lbs)</label></td>
                <td><input type="number" id="tractiveeffort" name="tractiveeffort" value="%TRACTIVEEFFORT%"
                  min="1000" max="100000" style=text-align:center></td>
                </tr>
                <tr>
                <td><label for="topspeed">Top Speed (mph)</label></td>
                <td><input type="number" id="topspeed" name="topspeed" value="%TOPSPEED%"
                  min="10" max="100" style=text-align:center></td>
                </tr>
            </table>
            <br><br>
            <input type="submit"> <input type="reset">
            </form>
            
        
        </div>
        
        </body>
        
        </html>
        )rawliteral";

const char functions_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en-US">
    
    <head>
    <link rel="stylesheet" href="stylesheet.css">
    <title>OLS Function Assignments</title>
    <meta charset="UTF-8">
    <meta name="author" content="George Hofmann">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    </head>
    
    <body>
    
    <div class="sidebar">
        <a href="index.html">Home</a>
        <a href="network.html">Network</a>
        <a href="locoparms.html">Locomotive</a>
        <a class="active" href="functions.html">Functions</a>
        <a href="calibration.html">Calibration</a>
        <a href="update/index.html">Update</a>
    </div>
    
    <div class="content">
        <h2>Function Assignments</h2>

        <p>Assign the function numbers associated with the actions listed.
            If there is no function available for an action, leave it blank.</p>
            
            <form action="/get">
            <input type="hidden" name="FunctionsParm" value="0">
            <table>
                <tr>
                <th style="text-align:left;">Action</th>
                <th>Function</th>
                </tr>
                <tr>
                <td><label for="headlight">Headlight</label></td>
                    <td><input type="number" id="headlight" name="headlight" value="%HEADLIGHT%" min="0" max="28" style=text-align:center></td>
                </tr>
                <tr>
                <td><label for="headlightdim">Headlight Dim</label></td>
                <td><input type="number" id="headlightdim" name="headlightdim" value="%HEADLIGHTDIM%" min="0" max="28" style=text-align:center></td>
                </tr>
                <tr>
                    <td><label for="rearlight">Rearlight</label></td>
                    <td><input type="number" id="rearlight" name="rearlight" value="%REARLIGHT%" min="0" max="28" style=text-align:center></td>
                    </tr>
                    <tr>
                    <td><label for="rearlightdim">Rearlight Dim</label></td>
                    <td><input type="number" id="rearlightdim" name="rearlightdim" value="%REARLIGHTDIM%" min="0" max="28" style=text-align:center></td>
                    </tr>
                <tr>
                <td><label for="bell">Bell</label></td>
                <td><input type="number" id="bell" name="bell" value="%BELL%" min="0" max="28" style=text-align:center></td>
                </tr>
                <tr>
                    <td><label for="tooter">Horn</label></td>
                    <td><input type="number" id="tooter" name="horn" value="%HORN%" min="0" max="28" style=text-align:center><br></td>
                </tr>
                <tr>
                    <td><label for="ibrake">Independent Brake</label></td>
                    <td><input type="number" id="ibrake" name="ibrake" value="%IBRAKE%" min="0" max="28" style=text-align:center><br></td>
                    </tr>
                    <tr>
                    <td><label for="tbrake">Train Brake</label></td>
                    <td><input type="number" id="tbrake" name="tbrake" value="%TBRAKE%" min="0" max="28" style=text-align:center><br></td>
                    </tr>
                    <tr>
                    <td><label for="ebrake">Emergency Brake</label></td>
                    <td><input type="number" id="ebrake" name="ebrake" value="%EBRAKE%" min="0" max="28" style=text-align:center><br></td>
                    </tr>
                <tr>
                <td><label for="brakesqueal">Brake Squeal</label></td>
                    <td><input type="number" id="brakesqueal" name="brakesqueal" value="%BRAKESQUEAL%" min="0" max="28" style=text-align:center><br></td>
                    </tr>
                    <tr>
                    <td><label for="pm">Prime Mover</label></td>
                    <td><input type="number" id="pm" name="pm" value="%PM%" min="0" max="28" style=text-align:center><br></td>
                </tr>
                <tr>
                    <td><label for="compressor">Compressor</label></td>
                    <td><input type="number" id="compressor" name="compressor" value="%COMPRESSOR%" min="0" max="28" style=text-align:center></td>
                </tr>
                <tr>
                    <td><label for="manNotch">Manual Notching</label></td>
                    <td><input type="number" id="manNotch" name="notchingenable" value="%NOTCHINGENABLE%" min="0" max="28" style=text-align:center></td>
                </tr>
                <tr>
                    <td><label for="notchUp">Notch Up</label></td>
                    <td><input type="number" id="notchUp" name="notchup" value="%NOTCHUP%" min="0" max="28" style=text-align:center></td>
                </tr>
                <tr>
                    <td><label for="notchDown">Notch Down</label></td>
                    <td><input type="number" id="notchDown" name="notchdown" value="%NOTCHDOWN%" min="0" max="28" style=text-align:center></td>
                    </tr>
                    </table>
                    <br><br>
                    <input type="submit">   <input type="reset">
        </form>
        <br>
        <hr>
        <h2>Set CV Values</h2>
        <p></p>
        <form action="/get">
        <input type="hidden" name="CvParm" value="0">
            <table>
                <tr>
                <td><label for="cv">CV</label></td>
                    <td><input type="number" id="cv" name="cv" value="%CV%" min="0" max="511" style=text-align:center></td>
                </tr>
                <tr>
                <td><label for="cvValue">CV Value</label></td>
                <td><input type="number" id="cvValue" name="cvValue" value="%CVVALUE%" min="0" max="255" style=text-align:center></td>
                </tr>
                
                </table>
                <br><br>
                <input type="submit"> 
                </form>
        <br>
        <hr>
        <h2>Function Labels</h2>
        <p></p>
        <form action="/get">
        <input type="hidden" name="FunctionLabels" value="0">
            <table>
                <tr>
                <th style="text-align:left;">Function</th>
                    <th>Label</th>
                    <tr>
                    <td><label for="f0">0</label></td>
                        <td><input type="text" id="f0" name="f0" value="%F0%"  style=text-align:center></td>
                        </tr>
                        <tr>
                        <td><label for="f1">1</label></td>
                        <td><input type="text" id="f1" name="f1" value="%F1%"  style=text-align:center></td>
                        </tr>
                        <tr>
                        <td><label for="f2">2</label></td>
                        <td><input type="text" id="f2" name="f2" value="%F2%"  style=text-align:center></td>
                    </tr>
                    <tr>
                    <td><label for="f3">3</label></td>
                        <td><input type="text" id="f3" name="f3" value="%F3%"  style=text-align:center></td>
                        </tr>
                        <tr>
                        <td><label for="f4">4</label></td>
                        <td><input type="text" id="f4" name="f4" value="%F4%"  style=text-align:center></td>
                        </tr>
                        <tr>
                        <td><label for="f5">5</label></td>
                        <td><input type="text" id="f5" name="f5" value="%F5%"  style=text-align:center></td>
                    </tr>
                    <tr>
                    <td><label for="f6">6</label></td>
                        <td><input type="text" id="f6" name="f6" value="%F6%"  style=text-align:center></td>
                        </tr>
                        <tr>
                        <td><label for="f7">7</label></td>
                        <td><input type="text" id="f7" name="f7" value="%F7%"  style=text-align:center></td>
                        </tr>
                    <tr>
                        <td><label for="f8">8</label></td>
                        <td><input type="text" id="f8" name="f8" value="%F8%"  style=text-align:center></td>
                        </tr>
                    <tr>
                    <td><label for="f9">9</label></td>
                    <td><input type="text" id="f9" name="f9" value="%F9%"  style=text-align:center></td>
                    </tr>
                    <tr>
                    <td><label for="f10">10</label></td>
                    <td><input type="text" id="f10" name="f10" value="%F10%"  style=text-align:center></td>
                    </tr>
                    <tr>
                    <td><label for="f11">11</label></td>
                        <td><input type="text" id="f11" name="f11" value="%F11%"  style=text-align:center></td>
                    </tr>
                    <tr>
                    <td><label for="f12">12</label></td>
                        <td><input type="text" id="f12" name="f12" value="%F12%"  style=text-align:center></td>
                        </tr>
                    <tr>
                    <td><label for="f13">13</label></td>
                        <td><input type="text" id="f13" name="f13" value="%F13%"  style=text-align:center></td>
                        </tr>
                    <tr>
                        <td><label for="f14">14</label></td>
                        <td><input type="text" id="f14" name="f14" value="%F14%"  style=text-align:center></td>
                        </tr>
                        <tr>
                        <td><label for="f15">15</label></td>
                        <td><input type="text" id="f15" name="f15" value="%F15%"  style=text-align:center></td>
                        </tr>
                        <tr>
                        <td><label for="f16">16</label></td>
                        <td><input type="text" id="f16" name="f16" value="%F16%"  style=text-align:center></td>
                    </tr>
                    <tr>
                    <td><label for="f17">17</label></td>
                        <td><input type="text" id="f17" name="f17" value="%F17%"  style=text-align:center></td>
                        </tr>
                        <tr>
                        <td><label for="f218">18</label></td>
                        <td><input type="text" id="f18" name="f18" value="%F18%"  style=text-align:center></td>
                    </tr>
                    <tr>
                        <td><label for="f19">19</label></td>
                        <td><input type="text" id="f19" name="f19" value="%F19%"  style=text-align:center></td>
                    </tr>
                    <tr>
                    <td><label for="f20">20</label></td>
                        <td><input type="text" id="f20" name="f20" value="%F20%"  style=text-align:center></td>
                    </tr>
                    <tr>
                        <td><label for="f21">21</label></td>
                        <td><input type="text" id="f21" name="f21" value="%F21%"  style=text-align:center></td>
                    </tr>
                    <tr>
                    <td><label for="f22">22</label></td>
                    <td><input type="text" id="f22" name="f22" value="%F22%"  style=text-align:center></td>
                    </tr>
                    <tr>
                        <td><label for="f23">23</label></td>
                        <td><input type="text" id="f23" name="f23" value="%F23%"  style=text-align:center></td>
                        </tr>
                        <tr>
                        <td><label for="f24">24</label></td>
                        <td><input type="text" id="f24" name="f24" value="%F24%"  style=text-align:center></td>
                        </tr>
                    <tr>
                    <td><label for="f25">25</label></td>
                    <td><input type="text" id="f25" name="f25" value="%F25%"  style=text-align:center></td>
                    </tr>
                    <tr>
                    <td><label for="f26">26</label></td>
                        <td><input type="text" id="f26" name="f26" value="%F26%"  style=text-align:center></td>
                    </tr>
                    <tr>
                        <td><label for="f27">27</label></td>
                        <td><input type="text" id="f27" name="f27" value="%F27%"  style=text-align:center></td>
                    </tr>
                    <tr>
                    <td><label for="f28">28</label></td>
                    <td><input type="text" id="f28" name="f28" value="%F28%"  style=text-align:center></td>
                    </tr>
                </tr>
                </table>
                <br><br>
                <input type="submit">   <input type="reset">
            <br><br>
            
            </form>
            </div>

</body>

</html>
)rawliteral";

const char calibration_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en-US">
    
    <head>
    <link rel="stylesheet" href="stylesheet.css">
    <title>OLS Speed Calibration</title>
    <meta charset="UTF-8">
    <meta name="author" content="George Hofmann">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
    input[type="number"]{
        width: 6em;
        }
        input::-webkit-outer-spin-button,
        input::-webkit-inner-spin-button {
            -webkit-appearance: none;
            margin: 0;
            }
            </style>
            </head>
            
<body>

<div class="sidebar">
<a href="index.html">Home</a>
        <a href="network.html">Network</a>
        <a href="locoparms.html">Locomotive</a>
        <a href="functions.html">Functions</a>
        <a class="active" href="calibration.html">Calibration</a>
        <a href="update/index.html">Update</a>
        </div>
        
    <div class="content">


    <h2>Locomotive Speed Calibration</h2>
    
    <p>These values are normally set by using the calibration routine from a throttle (timing through a trap).
            However the default values or those obtained using the speed trap can be modified here. </p>
            <p>Default trap lengths are:
            </p>
            <ul>
            <li>2 mph - 1 foot</li>
            <li>5 mph - 2 feet</li>
            <li>10 mph - 3 feet</li>
            <li>20 mph - 4 feet</li>
            <li>50 mph - 5 feet</li>
            </ul>
            <p>Start the loco moving using a speed button, click the button again at start of trap, click again at end of trap.
            The cal factor is computed and saved at end of trap. Click cancel at any point before end of trap in case of any fumble.
            The calibration should be done in both directions for each speed.
            </p>
            
            <form action="/get">
            <input type="hidden" name="calibrationParm" value="0">
            <table>
            <tr>
            <th></th>
            <th>Fwd</th>
            <th>Rev</th>
            </tr>
            <tr>
            <td><label for="speed2forward">2 mph</label></td>
            <td><input type="number" id="speed2forward" name="speed2forward" value="%SPEED2FORWARD%"  style=text-align:center min="-10.00" max="10.00" step=".01"></td>
            <td><input type="number" id="speed2reverse" name="speed2reverse" value="%SPEED2REVERSE%"  style=text-align:center min="-10.00" max="10.00" step=".01"><br></td>
            </tr>
            <tr>
            <td><label for="speed5forward">5 mph</label></td>
            <td><input type="number" id="speed5forward" name="speed5forward" value="%SPEED5FORWARD%"  style=text-align:center min="-10" max="10" step=".01"></td>
            <td><input type="number" id="speed5reverse" name="speed5reverse" value="%SPEED5REVERSE%"  style=text-align:center min="-10" max="10" step=".01"><br></td>
            </tr>
            <tr>
            <td><label for="speed10forward">10 mph</label></td>
            <td><input type="number" id="speed10forward" name="speed10forward" value="%SPEED10FORWARD%"  style=text-align:center min="-10" max="10" step=".01"></td>
            <td><input type="number" id="speed10reverse" name="speed10reverse" value="%SPEED10REVERSE%"  style=text-align:center min="-10" max="10" step=".01"><br></td>
            </tr>
            <tr>
            <td><label for="speed20forward">20 mph</label></td>
            <td><input type="number" id="speed20forward" name="speed20forward" value="%SPEED20FORWARD%"  style=text-align:center min="-10" max="10" step=".01"></td>
                    <td><input type="number" id="speed20reverse" name="speed20reverse" value="%SPEED20REVERSE%"  style=text-align:center min="-10" max="10" step=".01"><br></td>
                    </tr>
                    <tr>
                    <td><label for="speed50forward">50 mph</label></td>
                    <td><input type="number" id="speed50forward" name="speed50forward" value="%SPEED50FORWARD%"  style=text-align:center min="-10" max="10" step=".01"></td>
                    <td><input type="number" id="speed50reverse" name="speed50reverse" value="%SPEED50REVERSE%"  style=text-align:center min="-10" max="10" step=".01"><br></td>
                    </tr>
      
            </table>
            <br><br>
            <input type="submit">   <input type="reset">
        </form>

        </div>
        
</body>

</html>
)rawliteral";

const char stylesheet_css[] PROGMEM = R"rawliteral(
    
body {
    background-color:cornsilk;
    font-family:verdana;
    }
    
    /* The side navigation menu */
    .sidebar {
        margin: 0;
        padding: 0;
        width: 200px;
        background-color: #f1f1f1;
        position: fixed;
        height: 100%;
        overflow: auto;
        }
        
        /* Sidebar links */
        .sidebar a {
            display: block;
            color: black;
            padding: 16px;
            text-decoration: none;
            }
            
            /* Active/current link */
            .sidebar a.active {
                background-color: #04AA6D;
                color: white;
                }
                
                /* Links on mouse-over */
                .sidebar a:hover:not(.active) {
                    background-color: #555;
                    color: white;
                    }
                    
                    /* Page content. The value of the margin-left property should match the value of the sidebar's width property */
                    div.content {
                        margin-left: 200px;
                        padding: 1px 16px;
                        height: 1000px;
                        }
                        
                        /* On screens that are less than 700px wide, make the sidebar into a topbar */
                        @media screen and (max-width: 700px) {
                            .sidebar {
                                width: 100%;
                                height: auto;
    position: relative;
    }
    .sidebar a {float: left;}
    div.content {margin-left: 0;}
}

/* On screens that are less than 400px, display the bar vertically, instead of horizontally */
@media screen and (max-width: 400px) {
  .sidebar a {
    text-align: center;
    float: none;
  }
}

)rawliteral";

const char apselect_html[] PROGMEM = R"rawliteral(
<!-- apselect.html -->
<!doctype html>
<html>

<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">

  <title>WiFi Setup</title>
  <link rel="stylesheet" href="/stylesheet.css">
  <style>
    body {
      font-family: Arial, Helvetica, sans-serif;
      padding: 12px;
    }

    .container {
      max-width: 600px;
      margin: auto;
    }

    .ap {
      padding: 8px;
      border: 1px solid #ddd;
      margin: 6px 0;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }

    .status {
      margin-bottom: 12px;
      padding: 8px;
      background: #f7f7f7;
      border: 1px solid #eee;
    }

    input[type=password],
    input[type=text] {
      width: 100%;
      padding: 8px;
      margin-top: 6px;
      margin-bottom: 8px;
    }

    label {
      font-weight: 600;
    }

    .small {
      font-size: 0.9em;
      color: #666;
    }
  </style>
</head>

<body>
  <div class="container">
    <h2>Configure Wi-Fi</h2>
    <div id="status" class="status">Checking status...</div>

    <div class="card">
      <label for="ssid">Network Name (SSID)</label>
      <input id="ssid" type="text" placeholder="Select from list or type here" />
      <label for="password">Password</label>
      <input id="password" type="password" placeholder="Enter password" />
      <button id="saveBtn" class="btn-save">Save & Connect</button>
    </div>

    <hr>

    <div class="header-row">
      <h3>Available Networks</h3>
      <button id="scanBtn" class="btn-scan">Scan for Networks</button>
    </div>
    <div id="aplist" class="ap-list">Click "Scan" to find networks...</div>

    <p class="small">Portal active for 3 minutes. <a href="/index.html">Exit to Index</a></p>
  </div>

  <script>
    const ssidInput = document.getElementById('ssid');
    const passInput = document.getElementById('password');
    const aplistDiv = document.getElementById('aplist');
    const scanBtn = document.getElementById('scanBtn');

    function updateStatus() {
      fetch('/status').then(r => r.json()).then(js => {
        const st = document.getElementById('status');
        st.innerHTML = js.connected ? 
          `✅ Connected: <b>${js.ssid}</b>` : 
          `❌ <b>Not connected</b>`;
      }).catch(() => {});
    }

    function doScan() {
      scanBtn.disabled = true;
      scanBtn.innerText = "Scanning...";
      aplistDiv.innerHTML = '<div class="spinner">Scanning channels... please wait.</div>';

      fetch('/aplist')
        .then(r => r.json())
        .then(list => {
          aplistDiv.innerHTML = '';
          if (list.length === 0) {
            aplistDiv.innerText = 'No networks found. Try again.';
          } else {
            list.sort((a, b) => b.rssi - a.rssi);
            list.forEach(item => {
              let div = document.createElement('div');
              div.className = 'ap-item';
              div.innerHTML = `<span><b>${item.ssid}</b> (${item.rssi}dBm)</span>`;
              let btn = document.createElement('button');
              btn.textContent = 'Select';
              btn.onclick = () => { ssidInput.value = item.ssid; passInput.focus(); };
              div.appendChild(btn);
              aplistDiv.appendChild(div);
            });
          }
        })
        .finally(() => {
          scanBtn.disabled = false;
          scanBtn.innerText = "Scan for Networks";
        });
    }

    document.getElementById('saveBtn').onclick = function () {
      if (!ssidInput.value) return alert('SSID required');
      
      this.disabled = true;
      this.innerText = "Saving...";

      const body = "ssid=" + encodeURIComponent(ssidInput.value) + 
                   "&password=" + encodeURIComponent(passInput.value);

      fetch('/connect', {
        method: 'POST',
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: body
      })
      .then(r => r.json())
      .then(js => {
        alert(js.message || "Rebooting...");
        // Wait 2 seconds then try to redirect to home
        setTimeout(() => { window.location.href = "/"; }, 2000);
      })
      .catch(e => alert("Save request sent. Device is likely rebooting."));
    };

    scanBtn.onclick = doScan;
    window.onload = updateStatus;
  </script>
</body>



</html>
    )rawliteral";
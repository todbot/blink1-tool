
// Part of https://todbot.github.io/blink1-tool/

let canvas = document.getElementById('canvas_picker').getContext('2d');
let rgbinput = document.getElementById('rgb');
let hexinput = document.getElementById('hex');
let leds = document.getElementById('leds');
let status = document.getElementById('status');
let canvas_picker = document.getElementById('canvas_picker');
canvas_picker.addEventListener('click', handleClick);
canvas_picker.addEventListener('mousemove', handleClick);
canvas_picker.addEventListener('touchmove', handleClick);
document.addEventListener('DOMContentLoaded', handleDOMContentLoaded);

// create an image object and set its source
let img = new Image();
img.src = 'HTML-Color-Code-300x255.gif';

var pending = null;

// copy the image to the canvas
img.addEventListener('load', function() {
    canvas.drawImage(img,0,0);
});

async function handleDOMContentLoaded() {
    try {
        const response = await fetch('/blink1/id');
        if (!response.ok) {
            throw new Error('HTTP error! status: ' + response.status);
        }
        const results = await response.json()
        const serialnums = results.blink1_serialnums;
        for (const i in serialnums) {
            const serialnum = serialnums[i];
            const option = document.createElement('option');
            option.value = 'id=' + serialnum;
            option.text = 'serial: ' + serialnum;
            leds.add(option);
            if (serialnum && serialnum[0] > '1') {
                for (let ledn = 1; ledn <= 2; ledn++) {
                    const option = document.createElement('option');
                    option.value = 'id=' + serialnum + '&ledn=' + ledn;
                    option.text = 'serial: ' + serialnum + ', only led #' + ledn;
                    leds.add(option);
                }
            }
        }
        if( serialnums && serialnums.length > 0 ) {
            status.textContent = "connected";
        } else {
            status.textContent = "no devices";
        }
    } catch (error) {
        console.error('handleDOMContentLoaded: failed:', error);
    }
}

// http://www.javascripter.net/faq/rgbtohex.htm
function rgbToHex(R,G,B) { return toHex(R)+toHex(G)+toHex(B); }
function toHex(n) {
    n = parseInt(n,10);
    if (isNaN(n)) return "00";
    n = Math.max(0,Math.min(n,255));
    return "0123456789ABCDEF".charAt((n-n%16)/16)  + "0123456789ABCDEF".charAt(n%16);
}

async function handleClick(event) {
    // get click coordinates in image space
    const [x, y] = function() {
        if (event.type == "touchmove") {
            event.preventDefault();
            const boundingClientRect = event.target.getBoundingClientRect()
            return [
                event.changedTouches[0].clientX - boundingClientRect.x,
                event.changedTouches[0].clientY - boundingClientRect.y,
            ];
        }
        return [event.offsetX, event.offsetY];
    }();
    // get image data and RGB values
    const img_data = canvas.getImageData(x, y, 1, 1).data;
    const r = img_data[0];
    const g = img_data[1];
    const b = img_data[2];
    const rgbstr = r + ',' + g + ',' + b;
    const hexstr = rgbToHex(r,g,b);

    if ( rgbinput.value == rgbstr ) return;
    rgbinput.value = rgbstr;
    hexinput.value = '#' + hexstr;
    console.log("hex:",hexstr);

    await fadeToColor([r,g,b], 100, 0 );
}

async function fadeToColor([r, g, b], fadeMillis, ledn ) {
    const f = (x) => 256 * (x / 256) ** 2;  // gamma of 0.5

    const searchParams = new URLSearchParams(location.search)
    new URLSearchParams(leds.value).forEach((value, key) => searchParams.set(key, value));
    searchParams.set('rgb', rgbToHex(f(r), f(g), f(b)))
    if (!searchParams.has('time')) searchParams.set('time', fadeMillis / 1000)
    if (!searchParams.has('ledn')) searchParams.set('ledn', ledn)

    const resource = '/blink1/fadeToRGB?' + searchParams;
    if ( pending ) {
        pending = resource;
        return;
    }
    pending = resource;
    setTimeout(fetchResource);
}

async function fetchResource() {
    let resource;
    while (true) {
        if ( resource && pending == resource ) {
            pending = null;
        }
        resource = pending;
        if ( !resource ) return;

        try {
            const response = await fetch(resource);
            if (!response.ok) {
                throw new Error('HTTP error! status: ' + response.status);
            }
            const results = await response.json()
            status.textContent = results.status;
        } catch (error) {
            console.error('fadeToColor: failed:', error);
        }
    }
}



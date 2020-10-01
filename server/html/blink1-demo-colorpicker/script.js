
// Part of https://todbot.github.io/blink1-tool/

let canvas = document.getElementById('canvas_picker').getContext('2d');
let rgbinput = document.getElementById('rgb');
let hexinput = document.getElementById('hex');
let canvas_picker = document.getElementById('canvas_picker');
canvas_picker.addEventListener('click', handleClick);
canvas_picker.addEventListener('mousemove', handleClick);

// create an image object and set its source
let img = new Image();
img.src = 'HTML-Color-Code-300x255.gif';

var pending = null;

// copy the image to the canvas
img.addEventListener('load', function() {
	  canvas.drawImage(img,0,0);
});

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
    const x = event.offsetX;
    const y = event.offsetY;
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
            await fetch(resource);
        } catch (error) {
            console.error('fadeToColor: failed:', error);
        }
    }
}



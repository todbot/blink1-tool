
// Part of https://todbot.github.io/blink1-tool/

document.getElementById('start-button').addEventListener('click', handleClick);
document.getElementById('stop-button').addEventListener('click', handleClickStop);

let timer;

async function handleClickStop() {
    clearTimeout(timer);
    timer = null;
    await fadeToColor([0,0,0], 100, 0);
}

async function handleClick() {
    if(timer) return;
    startParty();
}

async function startParty() {
    timer = setTimeout(startParty, 200);
    let acolor = randColor();
    const ledn = 1 + Math.floor(Math.random()*2);
    const fadeMillis = 100;
    await fadeToColor(acolor, fadeMillis, ledn );
}

// http://www.javascripter.net/faq/rgbtohex.htm
function rgbToHex(R,G,B) { return toHex(R)+toHex(G)+toHex(B); }
function toHex(n) {
    n = parseInt(n,10);
    if (isNaN(n)) return "00";
    n = Math.max(0,Math.min(n,255));
    return "0123456789ABCDEF".charAt((n-n%16)/16)  + "0123456789ABCDEF".charAt(n%16);
}

async function fadeToColor([r, g, b], fadeMillis, ledn ) {
    const f = (x) => 256 * (x / 256) ** 2;  // gamma of 0.5

    const searchParams = new URLSearchParams(location.search)
    searchParams.set('rgb', rgbToHex(f(r), f(g), f(b)))
    if (!searchParams.has('time')) searchParams.set('time', fadeMillis / 1000)
    if (!searchParams.has('ledn')) searchParams.set('ledn', ledn)

    const resource = '/blink1/fadeToRGB?' + searchParams;
    try {
        await fetch(resource);
    } catch (error) {
        console.error('fadeToColor: failed:', error);
    }
}

function randColor() {
    const r = Math.floor(Math.random() * 255);
    const g = Math.floor(Math.random() * 255);
    const b = Math.floor(Math.random() * 255);
    return [r,g,b];
}

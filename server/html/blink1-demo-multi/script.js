
// Part of https://todbot.github.io/blink1-tool/

const swatchColors = [
    '#000000', '#ff0000', '#ff8000', '#ffff00', '#00ff00',
    '#00ffff', '#0000ff', '#8000ff', '#ff00ff', '#ffffff',
];

let status = document.getElementById('status');
let tbody = document.getElementById('blink1s');
let allSwatches = document.getElementById('all-swatches');

document.getElementById('rescan-button').addEventListener('click', refresh);
allSwatches.appendChild(makeSwatches(null, handleAllColor));
let blink1s = [];  // { serial, color }

refresh();

function makeSwatches(selected, onPick) {
    const span = document.createElement('span');
    for( const hex of swatchColors ) {
        const button = document.createElement('button');
        button.title = hex;
        button.style.cssText = `background:${hex}; width:24px; height:24px;
            margin:1px; border:${hex === selected ? '3px solid #888' : '1px solid #ccc'};`;
        button.addEventListener('click', () => onPick(hex));
        span.appendChild(button);
    }
    return span;
}

async function refresh() {
    try {
        const response = await fetch('/blink1/id');
        if (!response.ok) {
            throw new Error('HTTP error! status: ' + response.status);
        }
        const results = await response.json();
        const serials = results.blink1_serialnums || [];
        blink1s = serials.map(serial =>
            blink1s.find(b => b.serial === serial) || { serial, color: '#000000' });
    } catch (error) {
        console.error('refresh: failed:', error);
        blink1s = [];
    }
    render();
}

function render() {
    tbody.innerHTML = '';
    blink1s.forEach((b, i) => {
        const tr = document.createElement('tr');
        tr.innerHTML = `<td>${i}</td><td>${b.serial}</td>
          <td></td>
          <td><button> Identify </button></td>`;
        tr.querySelector('button').addEventListener('click', () => identify(b));
        tr.children[2].appendChild(makeSwatches(b.color, async (hex) => {
            await setColor(b, hex);
            render();
        }));
        tbody.appendChild(tr);
    });
    status.textContent = blink1s.length + " blink(1)s";
}

async function fadeToColor(serial, hex, fadeMillis) {
    const searchParams = new URLSearchParams({
        id: serial,
        rgb: hex.slice(1),
        // server treats 0 as its default fade time
        millis: Math.max(fadeMillis, 1),
    });
    try {
        await fetch('/blink1/fadeToRGB?' + searchParams);
    } catch (error) {
        console.error('fadeToColor: failed:', error);
    }
}

async function setColor(b, hex) {
    b.color = hex;
    await fadeToColor(b.serial, hex, 100);
}

async function handleAllColor(hex) {
    for( const b of blink1s ) {
        await setColor(b, hex);
    }
    render();
}

// flash white then go back to its color, to tell which one is which
async function identify(b) {
    await fadeToColor(b.serial, '#ffffff', 0);
    setTimeout(() => fadeToColor(b.serial, b.color, 300), 500);
}

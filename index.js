var view_id = 0;
var pixels;
var mapped_buffer;

function CallHandler(id, command, info)
{
    Module.ccall('PostJsMessage', null, ['number', 'string', 'string', 'string'], [view_id, id, command, info]);
}

function LoadWebView(sender, view_info, html, waves)
{
    view_id = sender;
    document.getElementById('image_view').style.display= 'none';
    document.getElementById('web_view').onload = function()
    {
        document.getElementById('web_view').contentWindow.CallHandler = CallHandler;
        document.getElementById('web_view').style.display = 'block';
        LoadView(view_info, UTF8ToString(waves));
        CallHandler('body', 'ready', '');
    };
    document.getElementById('web_view').data = 'assets/html/' + UTF8ToString(html) + '.htm';
}

function LoadImageView(sender, view_info, image_width, waves)
{
    view_id = sender;
    document.getElementById('web_view').style.display = 'none';
    document.getElementById('web_view').data = '';
    document.getElementById('image_view').style.display = 'block';
    var image_height = Math.floor(image_width *
        document.getElementById('image_view').height /
        document.getElementById('image_view').width);
    document.getElementById('image_view').width = image_width;
    document.getElementById('image_view').height = image_height;
    var ctx = document.getElementById('image_view').getContext("2d");
    pixels = ctx.createImageData(image_width, image_height);
    var pixelsData = Module.ccall('CreatePixels', null, ['number', 'number'], [image_width, image_height]);    
    mapped_buffer = new Uint8ClampedArray(Module.HEAPU8.buffer, pixelsData, image_width * image_height * 4);
    LoadView(view_info, UTF8ToString(waves));
    CallHandler('body', 'ready', image_width / 10 + ' ' + image_width + ' ' + image_height + ' ' + 33619971);
}

function LoadView(view_info, waves)
{
    if ((view_info & 8) != 0)
    {
        document.getElementById('escape').hidden = false;
    }
    else
    {
        document.getElementById('escape').hidden = true;
    }
    document.getElementById('audios').innerHTML = '';
    if (waves != '')
    {
        var audios = waves.split(' ');
        for (var i = 0; i < audios.length; ++i)
        {
            var audio = document.createElement('audio');
            audio.id = 'audio-' + i;
            audio.src = 'assets/wave/' + audios[i] + '.wav';
            audio.type = 'audio/wave';
            document.getElementById('audios').appendChild(audio);
        }
    }
}

function RefreshImageView()
{
    var ctx = document.getElementById('image_view').getContext("2d");
    pixels.data.set(mapped_buffer);
    ctx.putImageData(pixels, 0, 0);
}

function ImageMouseDown(event)
{
    var x = Math.floor(event.clientX * document.getElementById('image_view').width /
        document.getElementById('image_view').offsetWidth);
    var y = Math.floor(event.clientY * document.getElementById('image_view').height /
        document.getElementById('image_view').offsetHeight);
    CallHandler("body", "touch-begin", x + ' ' + y);
}

function ImageMouseMove(event)
{
    var x = Math.floor(event.clientX * document.getElementById('image_view').width /
        document.getElementById('image_view').offsetWidth);
    var y = Math.floor(event.clientY * document.getElementById('image_view').height /
        document.getElementById('image_view').offsetHeight);
    CallHandler("body", "touch-move", x + ' ' + y);
}

function ImageMouseUp(event)
{
    var x = Math.floor(event.clientX * document.getElementById('image_view').width /
        document.getElementById('image_view').offsetWidth);
    var y = Math.floor(event.clientY * document.getElementById('image_view').height /
        document.getElementById('image_view').offsetHeight);
    CallHandler("body", "touch-end", x + ' ' + y);
}

function Escape()
{
    Module.ccall('NeedEscape', null, null, null);
}

window.onbeforeunload = function()
{
    Module.ccall('NeedExit', null, null, null);
}
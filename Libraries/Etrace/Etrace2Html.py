#!/usr/bin/env python
import sys, os, getopt, time, socket, math
from collections import deque
# -*- coding: utf-8 -*-

html_template = r"""<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
<title>{{TITLE}}</title>
<meta http-equiv="Content-Type" content="text/html;charset=utf-8" />

<style type="text/css" >
body { font-family: Verdana; background-color: #ffffFF; font-size: 10px; }
h2 { font-size: 14px; font-weight: bold; background-color: #448; color: #eee; padding: 2px 2px 2px 20px; text-align: left; margin: 0px; }
h3 { font-size: 12px; font-weight: bold; padding: 20px 0px 0px 0px; text-align: left; }
.scroll-caption { border: 1px solid #ddc; background-color: #ffffe0; padding: 0px; margin: 0px; }
.scroll { overflow-x: scroll; display: block; }
.scroll-fill { height: 1; width: 1; float: left; }
#cnv { overflow: auto; position: relative; }
.button { width: 100px; padding-left: 10px; padding-right: 10px; }
.spacer { width: 100px; border: 0px; float: left; padding: 0px; height: 1px; }
.stat { font-size: 12px; text-align: center; background: white; font-family: Arial; padding: 0px; }
.clr0 td { color: green; font-weight: bold; }
.clr1 td { color: red;  font-weight: bold; }
.stat th { padding: 1px 0px 1px 0px; font-weight: bold;
	border-bottom: 1px solid #ccc; border-right: 1px solid #ccc;
	background: #eee; font-weight: normal; }
.stat td { padding: 1px 0px 1px 0px;
	border-bottom: 1px solid #ccc; border-right: 1px solid #ccc; background: #f8f8f8; }
th.caption { font-size: 12px; background-color: #ddd; font-weight: bold; text-align: left; padding-left: 20px; width: 610px; }

.ctrl td { padding: 0px 0px 0px 10px; font-size: 10px; background-color: #ffffe0; }
.editable td { background-color: #ffffe0; }
.active td { background-color: #ffffff; }
.shadows { border-bottom: 1px solid #ccc; border-right: 1px solid #ccc;
	border-left: 1px solid #f8f8f8; }
td.log { border: 1px solid #ccc; }
td.log pre { overflow-y: scroll; height: 275; text-align: left; margin: 2px; width: 700; }
td.log pre p.write { background-color: #ff8080; padding: 0px; margin: 0px; }
td.log pre p.read { background-color: #80EE80; padding: 0px; margin: 0px; }
td.log pre p.pcache { background-color: #9AFF90; padding: 0px; margin: 0px; }
#div_plot { padding-bottom: 5px; }
#div_info { padding: 5px; }
.hidden { display: block; }

a.dsphead{ text-decoration:none; }
a.dsphead:hover{ text-decoration:underline;}
a.dsphead span.dspchar{ font-family:monospace; font-weight:normal;}
.dspcont{ display:none; margin: 10px 0px 10px 20px; text-align: left; }

</style>

<script type="text/javascript">
function dsp(loc)
{
	if(document.getElementById)
	{
		var foc=loc.firstChild;
		foc=loc.firstChild.innerHTML ? loc.firstChild : loc.firstChild.nextSibling;
		foc.innerHTML=foc.innerHTML=='[+]'?'[-]':'[+]';
		foc=loc.parentNode.nextSibling.style ? loc.parentNode.nextSibling: loc.parentNode.nextSibling.nextSibling;
		foc.style.display = foc.style.display == 'block' ? 'none' : 'block';
	}
}
</script>
<script type="text/javascript">

var G_vmlCanvasManager;

/*****************************************************************
 * HTML5 canvas emulation for Internet Explorer.
 * It uses VML to implement required functionality
 *
 * Performace will suck in IE, but it's better than nothing...
 *****************************************************************/

// Copyright 2006 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


// Known Issues:
//
// * Patterns only support repeat.
// * Radial gradient are not implemented. The VML version of these look very
//   different from the canvas one.
// * Clipping paths are not implemented.
// * Coordsize. The width and height attribute have higher priority than the
//   width and height style values which isn't correct.
// * Painting mode isn't implemented.
// * Canvas width/height should is using content-box by default. IE in
//   Quirks mode will draw the canvas using border-box. Either change your
//   doctype to HTML5
//   (http://www.whatwg.org/specs/web-apps/current-work/#the-doctype)
//   or use Box Sizing Behavior from WebFX
//   (http://webfx.eae.net/dhtml/boxsizing/boxsizing.html)
// * Non uniform scaling does not correctly scale strokes.
// * Optimize. There is always room for speed improvements.

// Only add this code if we do not already have a canvas implementation
if (!document.createElement('canvas').getContext) {

(function() {

  // alias some functions to make (compiled) code shorter
  var m = Math;
  var mr = m.round;
  var ms = m.sin;
  var mc = m.cos;
  var abs = m.abs;
  var sqrt = m.sqrt;

  // this is used for sub pixel precision
  var Z = 10;
  var Z2 = Z / 2;

  var IE_VERSION = +navigator.userAgent.match(/MSIE ([\d.]+)?/)[1];

  /**
   * This funtion is assigned to the <canvas> elements as element.getContext().
   * @this {HTMLElement}
   * @return {CanvasRenderingContext2D_}
   */
  function getContext() {
    return this.context_ ||
        (this.context_ = new CanvasRenderingContext2D_(this));
  }

  var slice = Array.prototype.slice;

  /**
   * Binds a function to an object. The returned function will always use the
   * passed in {@code obj} as {@code this}.
   *
   * Example:
   *
   *   g = bind(f, obj, a, b)
   *   g(c, d) // will do f.call(obj, a, b, c, d)
   *
   * @param {Function} f The function to bind the object to
   * @param {Object} obj The object that should act as this when the function
   *     is called
   * @param {*} var_args Rest arguments that will be used as the initial
   *     arguments when the function is called
   * @return {Function} A new function that has bound this
   */
  function bind(f, obj, var_args) {
    var a = slice.call(arguments, 2);
    return function() {
      return f.apply(obj, a.concat(slice.call(arguments)));
    };
  }

  function encodeHtmlAttribute(s) {
    return String(s).replace(/&/g, '&amp;').replace(/"/g, '&quot;');
  }

  function addNamespace(doc, prefix, urn) {
    if (!doc.namespaces[prefix]) {
      doc.namespaces.add(prefix, urn, '#default#VML');
    }
  }

  function addNamespacesAndStylesheet(doc) {
    addNamespace(doc, 'g_vml_', 'urn:schemas-microsoft-com:vml');
    addNamespace(doc, 'g_o_', 'urn:schemas-microsoft-com:office:office');

    // Setup default CSS.  Only add one style sheet per document
    if (!doc.styleSheets['ex_canvas_']) {
      var ss = doc.createStyleSheet();
      ss.owningElement.id = 'ex_canvas_';
      ss.cssText = 'canvas{display:inline-block;overflow:hidden;' +
          // default size is 300x150 in Gecko and Opera
          'text-align:left;width:300px;height:150px}';
    }
  }

  // Add namespaces and stylesheet at startup.
  addNamespacesAndStylesheet(document);

  var G_vmlCanvasManager_ = {
    init: function(opt_doc) {
      var doc = opt_doc || document;
      // Create a dummy element so that IE will allow canvas elements to be
      // recognized.
      doc.createElement('canvas');
      doc.attachEvent('onreadystatechange', bind(this.init_, this, doc));
    },

    init_: function(doc) {
      // find all canvas elements
      var els = doc.getElementsByTagName('canvas');
      for (var i = 0; i < els.length; i++) {
        this.initElement(els[i]);
      }
    },

    /**
     * Public initializes a canvas element so that it can be used as canvas
     * element from now on. This is called automatically before the page is
     * loaded but if you are creating elements using createElement you need to
     * make sure this is called on the element.
     * @param {HTMLElement} el The canvas element to initialize.
     * @return {HTMLElement} the element that was created.
     */
    initElement: function(el) {
      if (!el.getContext) {
        el.getContext = getContext;

        // Add namespaces and stylesheet to document of the element.
        addNamespacesAndStylesheet(el.ownerDocument);

        // Remove fallback content. There is no way to hide text nodes so we
        // just remove all childNodes. We could hide all elements and remove
        // text nodes but who really cares about the fallback content.
        el.innerHTML = '';

        // do not use inline function because that will leak memory
        el.attachEvent('onpropertychange', onPropertyChange);
        el.attachEvent('onresize', onResize);

        var attrs = el.attributes;
        if (attrs.width && attrs.width.specified) {
          // TODO: use runtimeStyle and coordsize
          // el.getContext().setWidth_(attrs.width.nodeValue);
          el.style.width = attrs.width.nodeValue + 'px';
        } else {
          el.width = el.clientWidth;
        }
        if (attrs.height && attrs.height.specified) {
          // TODO: use runtimeStyle and coordsize
          // el.getContext().setHeight_(attrs.height.nodeValue);
          el.style.height = attrs.height.nodeValue + 'px';
        } else {
          el.height = el.clientHeight;
        }
        //el.getContext().setCoordsize_()
      }
      return el;
    }
  };

  function onPropertyChange(e) {
    var el = e.srcElement;

    switch (e.propertyName) {
      case 'width':
        el.getContext().clearRect();
        el.style.width = el.attributes.width.nodeValue + 'px';
        // In IE8 this does not trigger onresize.
        el.firstChild.style.width =  el.clientWidth + 'px';
        break;
      case 'height':
        el.getContext().clearRect();
        el.style.height = el.attributes.height.nodeValue + 'px';
        el.firstChild.style.height = el.clientHeight + 'px';
        break;
    }
  }

  function onResize(e) {
    var el = e.srcElement;
    if (el.firstChild) {
      el.firstChild.style.width =  el.clientWidth + 'px';
      el.firstChild.style.height = el.clientHeight + 'px';
    }
  }

  G_vmlCanvasManager_.init();

  // precompute "00" to "FF"
  var decToHex = [];
  for (var i = 0; i < 16; i++) {
    for (var j = 0; j < 16; j++) {
      decToHex[i * 16 + j] = i.toString(16) + j.toString(16);
    }
  }

  function createMatrixIdentity() {
    return [
      [1, 0, 0],
      [0, 1, 0],
      [0, 0, 1]
    ];
  }

  function matrixMultiply(m1, m2) {
    var result = createMatrixIdentity();

    for (var x = 0; x < 3; x++) {
      for (var y = 0; y < 3; y++) {
        var sum = 0;

        for (var z = 0; z < 3; z++) {
          sum += m1[x][z] * m2[z][y];
        }

        result[x][y] = sum;
      }
    }
    return result;
  }

  function copyState(o1, o2) {
    o2.fillStyle     = o1.fillStyle;
    o2.lineCap       = o1.lineCap;
    o2.lineJoin      = o1.lineJoin;
    o2.lineWidth     = o1.lineWidth;
    o2.miterLimit    = o1.miterLimit;
    o2.shadowBlur    = o1.shadowBlur;
    o2.shadowColor   = o1.shadowColor;
    o2.shadowOffsetX = o1.shadowOffsetX;
    o2.shadowOffsetY = o1.shadowOffsetY;
    o2.strokeStyle   = o1.strokeStyle;
    o2.globalAlpha   = o1.globalAlpha;
    o2.font          = o1.font;
    o2.textAlign     = o1.textAlign;
    o2.textBaseline  = o1.textBaseline;
    o2.arcScaleX_    = o1.arcScaleX_;
    o2.arcScaleY_    = o1.arcScaleY_;
    o2.lineScale_    = o1.lineScale_;
  }

  var colorData = {
    aliceblue: '#F0F8FF',
    antiquewhite: '#FAEBD7',
    aquamarine: '#7FFFD4',
    azure: '#F0FFFF',
    beige: '#F5F5DC',
    bisque: '#FFE4C4',
    black: '#000000',
    blanchedalmond: '#FFEBCD',
    blueviolet: '#8A2BE2',
    brown: '#A52A2A',
    burlywood: '#DEB887',
    cadetblue: '#5F9EA0',
    chartreuse: '#7FFF00',
    chocolate: '#D2691E',
    coral: '#FF7F50',
    cornflowerblue: '#6495ED',
    cornsilk: '#FFF8DC',
    crimson: '#DC143C',
    cyan: '#00FFFF',
    darkblue: '#00008B',
    darkcyan: '#008B8B',
    darkgoldenrod: '#B8860B',
    darkgray: '#A9A9A9',
    darkgreen: '#006400',
    darkgrey: '#A9A9A9',
    darkkhaki: '#BDB76B',
    darkmagenta: '#8B008B',
    darkolivegreen: '#556B2F',
    darkorange: '#FF8C00',
    darkorchid: '#9932CC',
    darkred: '#8B0000',
    darksalmon: '#E9967A',
    darkseagreen: '#8FBC8F',
    darkslateblue: '#483D8B',
    darkslategray: '#2F4F4F',
    darkslategrey: '#2F4F4F',
    darkturquoise: '#00CED1',
    darkviolet: '#9400D3',
    deeppink: '#FF1493',
    deepskyblue: '#00BFFF',
    dimgray: '#696969',
    dimgrey: '#696969',
    dodgerblue: '#1E90FF',
    firebrick: '#B22222',
    floralwhite: '#FFFAF0',
    forestgreen: '#228B22',
    gainsboro: '#DCDCDC',
    ghostwhite: '#F8F8FF',
    gold: '#FFD700',
    goldenrod: '#DAA520',
    grey: '#808080',
    greenyellow: '#ADFF2F',
    honeydew: '#F0FFF0',
    hotpink: '#FF69B4',
    indianred: '#CD5C5C',
    indigo: '#4B0082',
    ivory: '#FFFFF0',
    khaki: '#F0E68C',
    lavender: '#E6E6FA',
    lavenderblush: '#FFF0F5',
    lawngreen: '#7CFC00',
    lemonchiffon: '#FFFACD',
    lightblue: '#ADD8E6',
    lightcoral: '#F08080',
    lightcyan: '#E0FFFF',
    lightgoldenrodyellow: '#FAFAD2',
    lightgreen: '#90EE90',
    lightgrey: '#D3D3D3',
    lightpink: '#FFB6C1',
    lightsalmon: '#FFA07A',
    lightseagreen: '#20B2AA',
    lightskyblue: '#87CEFA',
    lightslategray: '#778899',
    lightslategrey: '#778899',
    lightsteelblue: '#B0C4DE',
    lightyellow: '#FFFFE0',
    limegreen: '#32CD32',
    linen: '#FAF0E6',
    magenta: '#FF00FF',
    mediumaquamarine: '#66CDAA',
    mediumblue: '#0000CD',
    mediumorchid: '#BA55D3',
    mediumpurple: '#9370DB',
    mediumseagreen: '#3CB371',
    mediumslateblue: '#7B68EE',
    mediumspringgreen: '#00FA9A',
    mediumturquoise: '#48D1CC',
    mediumvioletred: '#C71585',
    midnightblue: '#191970',
    mintcream: '#F5FFFA',
    mistyrose: '#FFE4E1',
    moccasin: '#FFE4B5',
    navajowhite: '#FFDEAD',
    oldlace: '#FDF5E6',
    olivedrab: '#6B8E23',
    orange: '#FFA500',
    orangered: '#FF4500',
    orchid: '#DA70D6',
    palegoldenrod: '#EEE8AA',
    palegreen: '#98FB98',
    paleturquoise: '#AFEEEE',
    palevioletred: '#DB7093',
    papayawhip: '#FFEFD5',
    peachpuff: '#FFDAB9',
    peru: '#CD853F',
    pink: '#FFC0CB',
    plum: '#DDA0DD',
    powderblue: '#B0E0E6',
    rosybrown: '#BC8F8F',
    royalblue: '#4169E1',
    saddlebrown: '#8B4513',
    salmon: '#FA8072',
    sandybrown: '#F4A460',
    seagreen: '#2E8B57',
    seashell: '#FFF5EE',
    sienna: '#A0522D',
    skyblue: '#87CEEB',
    slateblue: '#6A5ACD',
    slategray: '#708090',
    slategrey: '#708090',
    snow: '#FFFAFA',
    springgreen: '#00FF7F',
    steelblue: '#4682B4',
    tan: '#D2B48C',
    thistle: '#D8BFD8',
    tomato: '#FF6347',
    turquoise: '#40E0D0',
    violet: '#EE82EE',
    wheat: '#F5DEB3',
    whitesmoke: '#F5F5F5',
    yellowgreen: '#9ACD32'
  };


  function getRgbHslContent(styleString) {
    var start = styleString.indexOf('(', 3);
    var end = styleString.indexOf(')', start + 1);
    var parts = styleString.substring(start + 1, end).split(',');
    // add alpha if needed
    if (parts.length != 4 || styleString.charAt(3) != 'a') {
      parts[3] = 1;
    }
    return parts;
  }

  function percent(s) {
    return parseFloat(s) / 100;
  }

  function clamp(v, min, max) {
    return Math.min(max, Math.max(min, v));
  }

  function hslToRgb(parts){
    var r, g, b, h, s, l;
    h = parseFloat(parts[0]) / 360 % 360;
    if (h < 0)
      h++;
    s = clamp(percent(parts[1]), 0, 1);
    l = clamp(percent(parts[2]), 0, 1);
    if (s == 0) {
      r = g = b = l; // achromatic
    } else {
      var q = l < 0.5 ? l * (1 + s) : l + s - l * s;
      var p = 2 * l - q;
      r = hueToRgb(p, q, h + 1 / 3);
      g = hueToRgb(p, q, h);
      b = hueToRgb(p, q, h - 1 / 3);
    }

    return '#' + decToHex[Math.floor(r * 255)] +
        decToHex[Math.floor(g * 255)] +
        decToHex[Math.floor(b * 255)];
  }

  function hueToRgb(m1, m2, h) {
    if (h < 0)
      h++;
    if (h > 1)
      h--;

    if (6 * h < 1)
      return m1 + (m2 - m1) * 6 * h;
    else if (2 * h < 1)
      return m2;
    else if (3 * h < 2)
      return m1 + (m2 - m1) * (2 / 3 - h) * 6;
    else
      return m1;
  }

  function processStyle(styleString) {
    var str, alpha = 1;

    styleString = String(styleString);
    if (styleString.charAt(0) == '#') {
      str = styleString;
    } else if (/^rgb/.test(styleString)) {
      var parts = getRgbHslContent(styleString);
      var str = '#', n;
      for (var i = 0; i < 3; i++) {
        if (parts[i].indexOf('%') != -1) {
          n = Math.floor(percent(parts[i]) * 255);
        } else {
          n = +parts[i];
        }
        str += decToHex[clamp(n, 0, 255)];
      }
      alpha = +parts[3];
    } else if (/^hsl/.test(styleString)) {
      var parts = getRgbHslContent(styleString);
      str = hslToRgb(parts);
      alpha = parts[3];
    } else {
      str = colorData[styleString] || styleString;
    }
    return {color: str, alpha: alpha};
  }

  var DEFAULT_STYLE = {
    style: 'normal',
    variant: 'normal',
    weight: 'normal',
    size: 10,
    family: 'sans-serif'
  };

  // Internal text style cache
  var fontStyleCache = {};

  function processFontStyle(styleString) {
    if (fontStyleCache[styleString]) {
      return fontStyleCache[styleString];
    }

    var el = document.createElement('div');
    var style = el.style;
    try {
      style.font = styleString;
    } catch (ex) {
      // Ignore failures to set to invalid font.
    }

    return fontStyleCache[styleString] = {
      style: style.fontStyle || DEFAULT_STYLE.style,
      variant: style.fontVariant || DEFAULT_STYLE.variant,
      weight: style.fontWeight || DEFAULT_STYLE.weight,
      size: style.fontSize || DEFAULT_STYLE.size,
      family: style.fontFamily || DEFAULT_STYLE.family
    };
  }

  function getComputedStyle(style, element) {
    var computedStyle = {};

    for (var p in style) {
      computedStyle[p] = style[p];
    }

    // Compute the size
    var canvasFontSize = parseFloat(element.currentStyle.fontSize),
        fontSize = parseFloat(style.size);

    if (typeof style.size == 'number') {
      computedStyle.size = style.size;
    } else if (style.size.indexOf('px') != -1) {
      computedStyle.size = fontSize;
    } else if (style.size.indexOf('em') != -1) {
      computedStyle.size = canvasFontSize * fontSize;
    } else if(style.size.indexOf('%') != -1) {
      computedStyle.size = (canvasFontSize / 100) * fontSize;
    } else if (style.size.indexOf('pt') != -1) {
      computedStyle.size = fontSize / .75;
    } else {
      computedStyle.size = canvasFontSize;
    }

    // Different scaling between normal text and VML text. This was found using
    // trial and error to get the same size as non VML text.
    computedStyle.size *= 0.981;

    return computedStyle;
  }

  function buildStyle(style) {
    return style.style + ' ' + style.variant + ' ' + style.weight + ' ' +
        style.size + 'px ' + style.family;
  }

  function processLineCap(lineCap) {
    switch (lineCap) {
      case 'butt':
        return 'flat';
      case 'round':
        return 'round';
      case 'square':
      default:
        return 'square';
    }
  }

  /**
   * This class implements CanvasRenderingContext2D interface as described by
   * the WHATWG.
   * @param {HTMLElement} canvasElement The element that the 2D context should
   * be associated with
   */
  function CanvasRenderingContext2D_(canvasElement) {
    this.m_ = createMatrixIdentity();

    this.mStack_ = [];
    this.aStack_ = [];
    this.currentPath_ = [];

    // Canvas context properties
    this.strokeStyle = '#000';
    this.fillStyle = '#000';

    this.lineWidth = 1;
    this.lineJoin = 'miter';
    this.lineCap = 'butt';
    this.miterLimit = Z * 1;
    this.globalAlpha = 1;
    this.font = '10px sans-serif';
    this.textAlign = 'left';
    this.textBaseline = 'alphabetic';
    this.canvas = canvasElement;

    var cssText = 'width:' + canvasElement.clientWidth + 'px;height:' +
        canvasElement.clientHeight + 'px;overflow:hidden;position:absolute';
    var el = canvasElement.ownerDocument.createElement('div');
    el.style.cssText = cssText;
    canvasElement.appendChild(el);

    var overlayEl = el.cloneNode(false);
    // Use a non transparent background.
    overlayEl.style.backgroundColor = 'red';
    overlayEl.style.filter = 'alpha(opacity=0)';
    canvasElement.appendChild(overlayEl);

    this.element_ = el;
    this.arcScaleX_ = 1;
    this.arcScaleY_ = 1;
    this.lineScale_ = 1;
  }

  var contextPrototype = CanvasRenderingContext2D_.prototype;
  contextPrototype.clearRect = function() {
    if (this.textMeasureEl_) {
      this.textMeasureEl_.removeNode(true);
      this.textMeasureEl_ = null;
    }
    this.element_.innerHTML = '';
  };

  contextPrototype.beginPath = function() {
    // TODO: Branch current matrix so that save/restore has no effect
    //       as per safari docs.
    this.currentPath_ = [];
  };

  contextPrototype.moveTo = function(aX, aY) {
    var p = this.getCoords_(aX, aY);
    this.currentPath_.push({type: 'moveTo', x: p.x, y: p.y});
    this.currentX_ = p.x;
    this.currentY_ = p.y;
  };

  contextPrototype.lineTo = function(aX, aY) {
    var p = this.getCoords_(aX, aY);
    this.currentPath_.push({type: 'lineTo', x: p.x, y: p.y});

    this.currentX_ = p.x;
    this.currentY_ = p.y;
  };

  contextPrototype.bezierCurveTo = function(aCP1x, aCP1y,
                                            aCP2x, aCP2y,
                                            aX, aY) {
    var p = this.getCoords_(aX, aY);
    var cp1 = this.getCoords_(aCP1x, aCP1y);
    var cp2 = this.getCoords_(aCP2x, aCP2y);
    bezierCurveTo(this, cp1, cp2, p);
  };

  // Helper function that takes the already fixed cordinates.
  function bezierCurveTo(self, cp1, cp2, p) {
    self.currentPath_.push({
      type: 'bezierCurveTo',
      cp1x: cp1.x,
      cp1y: cp1.y,
      cp2x: cp2.x,
      cp2y: cp2.y,
      x: p.x,
      y: p.y
    });
    self.currentX_ = p.x;
    self.currentY_ = p.y;
  }

  contextPrototype.quadraticCurveTo = function(aCPx, aCPy, aX, aY) {
    // the following is lifted almost directly from
    // http://developer.mozilla.org/en/docs/Canvas_tutorial:Drawing_shapes

    var cp = this.getCoords_(aCPx, aCPy);
    var p = this.getCoords_(aX, aY);

    var cp1 = {
      x: this.currentX_ + 2.0 / 3.0 * (cp.x - this.currentX_),
      y: this.currentY_ + 2.0 / 3.0 * (cp.y - this.currentY_)
    };
    var cp2 = {
      x: cp1.x + (p.x - this.currentX_) / 3.0,
      y: cp1.y + (p.y - this.currentY_) / 3.0
    };

    bezierCurveTo(this, cp1, cp2, p);
  };

  contextPrototype.arc = function(aX, aY, aRadius,
                                  aStartAngle, aEndAngle, aClockwise) {
    aRadius *= Z;
    var arcType = aClockwise ? 'at' : 'wa';

    var xStart = aX + mc(aStartAngle) * aRadius - Z2;
    var yStart = aY + ms(aStartAngle) * aRadius - Z2;

    var xEnd = aX + mc(aEndAngle) * aRadius - Z2;
    var yEnd = aY + ms(aEndAngle) * aRadius - Z2;

    // IE won't render arches drawn counter clockwise if xStart == xEnd.
    if (xStart == xEnd && !aClockwise) {
      xStart += 0.125; // Offset xStart by 1/80 of a pixel. Use something
                       // that can be represented in binary
    }

    var p = this.getCoords_(aX, aY);
    var pStart = this.getCoords_(xStart, yStart);
    var pEnd = this.getCoords_(xEnd, yEnd);

    this.currentPath_.push({type: arcType,
                           x: p.x,
                           y: p.y,
                           radius: aRadius,
                           xStart: pStart.x,
                           yStart: pStart.y,
                           xEnd: pEnd.x,
                           yEnd: pEnd.y});

  };

  contextPrototype.rect = function(aX, aY, aWidth, aHeight) {
    this.moveTo(aX, aY);
    this.lineTo(aX + aWidth, aY);
    this.lineTo(aX + aWidth, aY + aHeight);
    this.lineTo(aX, aY + aHeight);
    this.closePath();
  };

  contextPrototype.strokeRect = function(aX, aY, aWidth, aHeight) {
    var oldPath = this.currentPath_;
    this.beginPath();

    this.moveTo(aX, aY);
    this.lineTo(aX + aWidth, aY);
    this.lineTo(aX + aWidth, aY + aHeight);
    this.lineTo(aX, aY + aHeight);
    this.closePath();
    this.stroke();

    this.currentPath_ = oldPath;
  };

  contextPrototype.fillRect = function(aX, aY, aWidth, aHeight) {
    var oldPath = this.currentPath_;
    this.beginPath();

    this.moveTo(aX, aY);
    this.lineTo(aX + aWidth, aY);
    this.lineTo(aX + aWidth, aY + aHeight);
    this.lineTo(aX, aY + aHeight);
    this.closePath();
    this.fill();

    this.currentPath_ = oldPath;
  };

  contextPrototype.createLinearGradient = function(aX0, aY0, aX1, aY1) {
    var gradient = new CanvasGradient_('gradient');
    gradient.x0_ = aX0;
    gradient.y0_ = aY0;
    gradient.x1_ = aX1;
    gradient.y1_ = aY1;
    return gradient;
  };

  contextPrototype.createRadialGradient = function(aX0, aY0, aR0,
                                                   aX1, aY1, aR1) {
    var gradient = new CanvasGradient_('gradientradial');
    gradient.x0_ = aX0;
    gradient.y0_ = aY0;
    gradient.r0_ = aR0;
    gradient.x1_ = aX1;
    gradient.y1_ = aY1;
    gradient.r1_ = aR1;
    return gradient;
  };

  contextPrototype.drawImage = function(image, var_args) {
    var dx, dy, dw, dh, sx, sy, sw, sh;

    // to find the original width we overide the width and height
    var oldRuntimeWidth = image.runtimeStyle.width;
    var oldRuntimeHeight = image.runtimeStyle.height;
    image.runtimeStyle.width = 'auto';
    image.runtimeStyle.height = 'auto';

    // get the original size
    var w = image.width;
    var h = image.height;

    // and remove overides
    image.runtimeStyle.width = oldRuntimeWidth;
    image.runtimeStyle.height = oldRuntimeHeight;

    if (arguments.length == 3) {
      dx = arguments[1];
      dy = arguments[2];
      sx = sy = 0;
      sw = dw = w;
      sh = dh = h;
    } else if (arguments.length == 5) {
      dx = arguments[1];
      dy = arguments[2];
      dw = arguments[3];
      dh = arguments[4];
      sx = sy = 0;
      sw = w;
      sh = h;
    } else if (arguments.length == 9) {
      sx = arguments[1];
      sy = arguments[2];
      sw = arguments[3];
      sh = arguments[4];
      dx = arguments[5];
      dy = arguments[6];
      dw = arguments[7];
      dh = arguments[8];
    } else {
      throw Error('Invalid number of arguments');
    }

    var d = this.getCoords_(dx, dy);

    var w2 = sw / 2;
    var h2 = sh / 2;

    var vmlStr = [];

    var W = 10;
    var H = 10;

    // For some reason that I've now forgotten, using divs didn't work
    vmlStr.push(' <g_vml_:group',
                ' coordsize="', Z * W, ',', Z * H, '"',
                ' coordorigin="0,0"' ,
                ' style="width:', W, 'px;height:', H, 'px;position:absolute;');

    // If filters are necessary (rotation exists), create them
    // filters are bog-slow, so only create them if abbsolutely necessary
    // The following check doesn't account for skews (which don't exist
    // in the canvas spec (yet) anyway.

    if (this.m_[0][0] != 1 || this.m_[0][1] ||
        this.m_[1][1] != 1 || this.m_[1][0]) {
      var filter = [];

      // Note the 12/21 reversal
      filter.push('M11=', this.m_[0][0], ',',
                  'M12=', this.m_[1][0], ',',
                  'M21=', this.m_[0][1], ',',
                  'M22=', this.m_[1][1], ',',
                  'Dx=', mr(d.x / Z), ',',
                  'Dy=', mr(d.y / Z), '');

      // Bounding box calculation (need to minimize displayed area so that
      // filters don't waste time on unused pixels.
      var max = d;
      var c2 = this.getCoords_(dx + dw, dy);
      var c3 = this.getCoords_(dx, dy + dh);
      var c4 = this.getCoords_(dx + dw, dy + dh);

      max.x = m.max(max.x, c2.x, c3.x, c4.x);
      max.y = m.max(max.y, c2.y, c3.y, c4.y);

      vmlStr.push('padding:0 ', mr(max.x / Z), 'px ', mr(max.y / Z),
                  'px 0;filter:progid:DXImageTransform.Microsoft.Matrix(',
                  filter.join(''), ", sizingmethod='clip');");

    } else {
      vmlStr.push('top:', mr(d.y / Z), 'px;left:', mr(d.x / Z), 'px;');
    }

    vmlStr.push(' ">' ,
                '<g_vml_:image src="', image.src, '"',
                ' style="width:', Z * dw, 'px;',
                ' height:', Z * dh, 'px"',
                ' cropleft="', sx / w, '"',
                ' croptop="', sy / h, '"',
                ' cropright="', (w - sx - sw) / w, '"',
                ' cropbottom="', (h - sy - sh) / h, '"',
                ' />',
                '</g_vml_:group>');

    this.element_.insertAdjacentHTML('BeforeEnd', vmlStr.join(''));
  };

  contextPrototype.stroke = function(aFill) {
    var lineStr = [];
    var lineOpen = false;

    var W = 10;
    var H = 10;

    lineStr.push('<g_vml_:shape',
                 ' filled="', !!aFill, '"',
                 ' style="position:absolute;width:', W, 'px;height:', H, 'px;"',
                 ' coordorigin="0,0"',
                 ' coordsize="', Z * W, ',', Z * H, '"',
                 ' stroked="', !aFill, '"',
                 ' path="');

    var newSeq = false;
    var min = {x: null, y: null};
    var max = {x: null, y: null};

    for (var i = 0; i < this.currentPath_.length; i++) {
      var p = this.currentPath_[i];
      var c;

      switch (p.type) {
        case 'moveTo':
          c = p;
          lineStr.push(' m ', mr(p.x), ',', mr(p.y));
          break;
        case 'lineTo':
          lineStr.push(' l ', mr(p.x), ',', mr(p.y));
          break;
        case 'close':
          lineStr.push(' x ');
          p = null;
          break;
        case 'bezierCurveTo':
          lineStr.push(' c ',
                       mr(p.cp1x), ',', mr(p.cp1y), ',',
                       mr(p.cp2x), ',', mr(p.cp2y), ',',
                       mr(p.x), ',', mr(p.y));
          break;
        case 'at':
        case 'wa':
          lineStr.push(' ', p.type, ' ',
                       mr(p.x - this.arcScaleX_ * p.radius), ',',
                       mr(p.y - this.arcScaleY_ * p.radius), ' ',
                       mr(p.x + this.arcScaleX_ * p.radius), ',',
                       mr(p.y + this.arcScaleY_ * p.radius), ' ',
                       mr(p.xStart), ',', mr(p.yStart), ' ',
                       mr(p.xEnd), ',', mr(p.yEnd));
          break;
      }


      // TODO: Following is broken for curves due to
      //       move to proper paths.

      // Figure out dimensions so we can do gradient fills
      // properly
      if (p) {
        if (min.x == null || p.x < min.x) {
          min.x = p.x;
        }
        if (max.x == null || p.x > max.x) {
          max.x = p.x;
        }
        if (min.y == null || p.y < min.y) {
          min.y = p.y;
        }
        if (max.y == null || p.y > max.y) {
          max.y = p.y;
        }
      }
    }
    lineStr.push(' ">');

    if (!aFill) {
      appendStroke(this, lineStr);
    } else {
      appendFill(this, lineStr, min, max);
    }

    lineStr.push('</g_vml_:shape>');

    this.element_.insertAdjacentHTML('beforeEnd', lineStr.join(''));
  };

  function appendStroke(ctx, lineStr) {
    var a = processStyle(ctx.strokeStyle);
    var color = a.color;
    var opacity = a.alpha * ctx.globalAlpha;
    var lineWidth = ctx.lineScale_ * ctx.lineWidth;

    // VML cannot correctly render a line if the width is less than 1px.
    // In that case, we dilute the color to make the line look thinner.
    if (lineWidth < 1) {
      opacity *= lineWidth;
    }

    lineStr.push(
      '<g_vml_:stroke',
      ' opacity="', opacity, '"',
      ' joinstyle="', ctx.lineJoin, '"',
      ' miterlimit="', ctx.miterLimit, '"',
      ' endcap="', processLineCap(ctx.lineCap), '"',
      ' weight="', lineWidth, 'px"',
      ' color="', color, '" />'
    );
  }

  function appendFill(ctx, lineStr, min, max) {
    var fillStyle = ctx.fillStyle;
    var arcScaleX = ctx.arcScaleX_;
    var arcScaleY = ctx.arcScaleY_;
    var width = max.x - min.x;
    var height = max.y - min.y;
    if (fillStyle instanceof CanvasGradient_) {
      // TODO: Gradients transformed with the transformation matrix.
      var angle = 0;
      var focus = {x: 0, y: 0};

      // additional offset
      var shift = 0;
      // scale factor for offset
      var expansion = 1;

      if (fillStyle.type_ == 'gradient') {
        var x0 = fillStyle.x0_ / arcScaleX;
        var y0 = fillStyle.y0_ / arcScaleY;
        var x1 = fillStyle.x1_ / arcScaleX;
        var y1 = fillStyle.y1_ / arcScaleY;
        var p0 = ctx.getCoords_(x0, y0);
        var p1 = ctx.getCoords_(x1, y1);
        var dx = p1.x - p0.x;
        var dy = p1.y - p0.y;
        angle = Math.atan2(dx, dy) * 180 / Math.PI;

        // The angle should be a non-negative number.
        if (angle < 0) {
          angle += 360;
        }

        // Very small angles produce an unexpected result because they are
        // converted to a scientific notation string.
        if (angle < 1e-6) {
          angle = 0;
        }
      } else {
        var p0 = ctx.getCoords_(fillStyle.x0_, fillStyle.y0_);
        focus = {
          x: (p0.x - min.x) / width,
          y: (p0.y - min.y) / height
        };

        width  /= arcScaleX * Z;
        height /= arcScaleY * Z;
        var dimension = m.max(width, height);
        shift = 2 * fillStyle.r0_ / dimension;
        expansion = 2 * fillStyle.r1_ / dimension - shift;
      }

      // We need to sort the color stops in ascending order by offset,
      // otherwise IE won't interpret it correctly.
      var stops = fillStyle.colors_;
      stops.sort(function(cs1, cs2) {
        return cs1.offset - cs2.offset;
      });

      var length = stops.length;
      var color1 = stops[0].color;
      var color2 = stops[length - 1].color;
      var opacity1 = stops[0].alpha * ctx.globalAlpha;
      var opacity2 = stops[length - 1].alpha * ctx.globalAlpha;

      var colors = [];
      for (var i = 0; i < length; i++) {
        var stop = stops[i];
        colors.push(stop.offset * expansion + shift + ' ' + stop.color);
      }

      // When colors attribute is used, the meanings of opacity and o:opacity2
      // are reversed.
      lineStr.push('<g_vml_:fill type="', fillStyle.type_, '"',
                   ' method="none" focus="100%"',
                   ' color="', color1, '"',
                   ' color2="', color2, '"',
                   ' colors="', colors.join(','), '"',
                   ' opacity="', opacity2, '"',
                   ' g_o_:opacity2="', opacity1, '"',
                   ' angle="', angle, '"',
                   ' focusposition="', focus.x, ',', focus.y, '" />');
    } else if (fillStyle instanceof CanvasPattern_) {
      if (width && height) {
        var deltaLeft = -min.x;
        var deltaTop = -min.y;
        lineStr.push('<g_vml_:fill',
                     ' position="',
                     deltaLeft / width * arcScaleX * arcScaleX, ',',
                     deltaTop / height * arcScaleY * arcScaleY, '"',
                     ' type="tile"',
                     // TODO: Figure out the correct size to fit the scale.
                     //' size="', w, 'px ', h, 'px"',
                     ' src="', fillStyle.src_, '" />');
       }
    } else {
      var a = processStyle(ctx.fillStyle);
      var color = a.color;
      var opacity = a.alpha * ctx.globalAlpha;
      lineStr.push('<g_vml_:fill color="', color, '" opacity="', opacity,
                   '" />');
    }
  }

  contextPrototype.fill = function() {
    this.stroke(true);
  };

  contextPrototype.closePath = function() {
    this.currentPath_.push({type: 'close'});
  };

  /**
   * @private
   */
  contextPrototype.getCoords_ = function(aX, aY) {
    var m = this.m_;
    return {
      x: Z * (aX * m[0][0] + aY * m[1][0] + m[2][0]) - Z2,
      y: Z * (aX * m[0][1] + aY * m[1][1] + m[2][1]) - Z2
    };
  };

  contextPrototype.save = function() {
    var o = {};
    copyState(this, o);
    this.aStack_.push(o);
    this.mStack_.push(this.m_);
    this.m_ = matrixMultiply(createMatrixIdentity(), this.m_);
  };

  contextPrototype.restore = function() {
    if (this.aStack_.length) {
      copyState(this.aStack_.pop(), this);
      this.m_ = this.mStack_.pop();
    }
  };

  function matrixIsFinite(m) {
    return isFinite(m[0][0]) && isFinite(m[0][1]) &&
        isFinite(m[1][0]) && isFinite(m[1][1]) &&
        isFinite(m[2][0]) && isFinite(m[2][1]);
  }

  function setM(ctx, m, updateLineScale) {
    if (!matrixIsFinite(m)) {
      return;
    }
    ctx.m_ = m;

    if (updateLineScale) {
      // Get the line scale.
      // Determinant of this.m_ means how much the area is enlarged by the
      // transformation. So its square root can be used as a scale factor
      // for width.
      var det = m[0][0] * m[1][1] - m[0][1] * m[1][0];
      ctx.lineScale_ = sqrt(abs(det));
    }
  }

  contextPrototype.translate = function(aX, aY) {
    var m1 = [
      [1,  0,  0],
      [0,  1,  0],
      [aX, aY, 1]
    ];

    setM(this, matrixMultiply(m1, this.m_), false);
  };

  contextPrototype.rotate = function(aRot) {
    var c = mc(aRot);
    var s = ms(aRot);

    var m1 = [
      [c,  s, 0],
      [-s, c, 0],
      [0,  0, 1]
    ];

    setM(this, matrixMultiply(m1, this.m_), false);
  };

  contextPrototype.scale = function(aX, aY) {
    this.arcScaleX_ *= aX;
    this.arcScaleY_ *= aY;
    var m1 = [
      [aX, 0,  0],
      [0,  aY, 0],
      [0,  0,  1]
    ];

    setM(this, matrixMultiply(m1, this.m_), true);
  };

  contextPrototype.transform = function(m11, m12, m21, m22, dx, dy) {
    var m1 = [
      [m11, m12, 0],
      [m21, m22, 0],
      [dx,  dy,  1]
    ];

    setM(this, matrixMultiply(m1, this.m_), true);
  };

  contextPrototype.setTransform = function(m11, m12, m21, m22, dx, dy) {
    var m = [
      [m11, m12, 0],
      [m21, m22, 0],
      [dx,  dy,  1]
    ];

    setM(this, m, true);
  };

  /**
   * The text drawing function.
   * The maxWidth argument isn't taken in account, since no browser supports
   * it yet.
   */
  contextPrototype.drawText_ = function(text, x, y, maxWidth, stroke) {
    var m = this.m_,
        delta = 1000,
        left = 0,
        right = delta,
        offset = {x: 0, y: 0},
        lineStr = [];

    var fontStyle = getComputedStyle(processFontStyle(this.font),
                                     this.element_);

    var fontStyleString = buildStyle(fontStyle);

    var elementStyle = this.element_.currentStyle;
    var textAlign = this.textAlign.toLowerCase();
    switch (textAlign) {
      case 'left':
      case 'center':
      case 'right':
        break;
      case 'end':
        textAlign = elementStyle.direction == 'ltr' ? 'right' : 'left';
        break;
      case 'start':
        textAlign = elementStyle.direction == 'rtl' ? 'right' : 'left';
        break;
      default:
        textAlign = 'left';
    }

    // 1.75 is an arbitrary number, as there is no info about the text baseline
    switch (this.textBaseline) {
      case 'hanging':
      case 'top':
        offset.y = fontStyle.size / 1.75;
        break;
      case 'middle':
        break;
      default:
      case null:
      case 'alphabetic':
      case 'ideographic':
      case 'bottom':
        offset.y = -fontStyle.size / 2.25;
        break;
    }

    switch(textAlign) {
      case 'right':
        left = delta;
        right = 0.05;
        break;
      case 'center':
        left = right = delta / 2;
        break;
    }

    var d = this.getCoords_(x + offset.x, y + offset.y);

    lineStr.push('<g_vml_:line from="', -left ,' 0" to="', right ,' 0.05" ',
                 ' coordsize="100 100" coordorigin="0 0"',
                 ' filled="', !stroke, '" stroked="', !!stroke,
                 '" style="position:absolute;width:1px;height:1px;">');

    if (stroke) {
      appendStroke(this, lineStr);
    } else {
      // TODO: Fix the min and max params.
      appendFill(this, lineStr, {x: -left, y: 0},
                 {x: right, y: fontStyle.size});
    }

    var skewM = m[0][0].toFixed(3) + ',' + m[1][0].toFixed(3) + ',' +
                m[0][1].toFixed(3) + ',' + m[1][1].toFixed(3) + ',0,0';

    var skewOffset = mr(d.x / Z) + ',' + mr(d.y / Z);

    lineStr.push('<g_vml_:skew on="t" matrix="', skewM ,'" ',
                 ' offset="', skewOffset, '" origin="', left ,' 0" />',
                 '<g_vml_:path textpathok="true" />',
                 '<g_vml_:textpath on="true" string="',
                 encodeHtmlAttribute(text),
                 '" style="v-text-align:', textAlign,
                 ';font:', encodeHtmlAttribute(fontStyleString),
                 '" /></g_vml_:line>');

    this.element_.insertAdjacentHTML('beforeEnd', lineStr.join(''));
  };

  contextPrototype.fillText = function(text, x, y, maxWidth) {
    this.drawText_(text, x, y, maxWidth, false);
  };

  contextPrototype.strokeText = function(text, x, y, maxWidth) {
    this.drawText_(text, x, y, maxWidth, true);
  };

  contextPrototype.measureText = function(text) {
    if (!this.textMeasureEl_) {
      var s = '<span style="position:absolute;' +
          'top:-20000px;left:0;padding:0;margin:0;border:none;' +
          'white-space:pre;"></span>';
      this.element_.insertAdjacentHTML('beforeEnd', s);
      this.textMeasureEl_ = this.element_.lastChild;
    }
    var doc = this.element_.ownerDocument;
    this.textMeasureEl_.innerHTML = '';
    this.textMeasureEl_.style.font = this.font;
    // Don't use innerHTML or innerText because they allow markup/whitespace.
    this.textMeasureEl_.appendChild(doc.createTextNode(text));
    return {width: this.textMeasureEl_.offsetWidth};
  };

  /******** STUBS ********/
  contextPrototype.clip = function() {
    // TODO: Implement
  };

  contextPrototype.arcTo = function() {
    // TODO: Implement
  };

  contextPrototype.createPattern = function(image, repetition) {
    return new CanvasPattern_(image, repetition);
  };

  // Gradient / Pattern Stubs
  function CanvasGradient_(aType) {
    this.type_ = aType;
    this.x0_ = 0;
    this.y0_ = 0;
    this.r0_ = 0;
    this.x1_ = 0;
    this.y1_ = 0;
    this.r1_ = 0;
    this.colors_ = [];
  }

  CanvasGradient_.prototype.addColorStop = function(aOffset, aColor) {
    aColor = processStyle(aColor);
    this.colors_.push({offset: aOffset,
                       color: aColor.color,
                       alpha: aColor.alpha});
  };

  function CanvasPattern_(image, repetition) {
    assertImageIsValid(image);
    switch (repetition) {
      case 'repeat':
      case null:
      case '':
        this.repetition_ = 'repeat';
        break
      case 'repeat-x':
      case 'repeat-y':
      case 'no-repeat':
        this.repetition_ = repetition;
        break;
      default:
        throwException('SYNTAX_ERR');
    }

    this.src_ = image.src;
    this.width_ = image.width;
    this.height_ = image.height;
  }

  function throwException(s) {
    throw new DOMException_(s);
  }

  function assertImageIsValid(img) {
    if (!img || img.nodeType != 1 || img.tagName != 'IMG') {
      throwException('TYPE_MISMATCH_ERR');
    }
    if (img.readyState != 'complete') {
      throwException('INVALID_STATE_ERR');
    }
  }

  function DOMException_(s) {
    this.code = this[s];
    this.message = s +': DOM Exception ' + this.code;
  }
  var p = DOMException_.prototype = new Error;
  p.INDEX_SIZE_ERR = 1;
  p.DOMSTRING_SIZE_ERR = 2;
  p.HIERARCHY_REQUEST_ERR = 3;
  p.WRONG_DOCUMENT_ERR = 4;
  p.INVALID_CHARACTER_ERR = 5;
  p.NO_DATA_ALLOWED_ERR = 6;
  p.NO_MODIFICATION_ALLOWED_ERR = 7;
  p.NOT_FOUND_ERR = 8;
  p.NOT_SUPPORTED_ERR = 9;
  p.INUSE_ATTRIBUTE_ERR = 10;
  p.INVALID_STATE_ERR = 11;
  p.SYNTAX_ERR = 12;
  p.INVALID_MODIFICATION_ERR = 13;
  p.NAMESPACE_ERR = 14;
  p.INVALID_ACCESS_ERR = 15;
  p.VALIDATION_ERR = 16;
  p.TYPE_MISMATCH_ERR = 17;

  // set up externs
  G_vmlCanvasManager = G_vmlCanvasManager_;
  CanvasRenderingContext2D = CanvasRenderingContext2D_;
  CanvasGradient = CanvasGradient_;
  CanvasPattern = CanvasPattern_;
  DOMException = DOMException_;
})();

} // if


/*****************************************************************
 * Some (lot of) browsers doesn't support canvas.fillText so far.
 * So we can emulate it by... lines :-) And it looks really great!
 *
 * CANVAS TEXT FUNCTIONS
 *****************************************************************/
//
// This code is released to the public domain by Jim Studt, 2007.
// He may keep some sort of up to date copy at http://www.federated.com/~jim/canvastext/
//

var CanvasTextFunctions = { };

CanvasTextFunctions.letters = {
    ' ': { width: 16, points: [] },
    '!': { width: 10, points: [[5,21],[5,7],[-1,-1],[5,2],[4,1],[5,0],[6,1],[5,2]] },
    '"': { width: 16, points: [[4,21],[4,14],[-1,-1],[12,21],[12,14]] },
    '#': { width: 21, points: [[11,25],[4,-7],[-1,-1],[17,25],[10,-7],[-1,-1],[4,12],[18,12],[-1,-1],[3,6],[17,6]] },
    '$': { width: 20, points: [[8,25],[8,-4],[-1,-1],[12,25],[12,-4],[-1,-1],[17,18],[15,20],[12,21],[8,21],[5,20],[3,18],[3,16],[4,14],[5,13],[7,12],[13,10],[15,9],[16,8],[17,6],[17,3],[15,1],[12,0],[8,0],[5,1],[3,3]] },
    '%': { width: 24, points: [[21,21],[3,0],[-1,-1],[8,21],[10,19],[10,17],[9,15],[7,14],[5,14],[3,16],[3,18],[4,20],[6,21],[8,21],[10,20],[13,19],[16,19],[19,20],[21,21],[-1,-1],[17,7],[15,6],[14,4],[14,2],[16,0],[18,0],[20,1],[21,3],[21,5],[19,7],[17,7]] },
    '&': { width: 26, points: [[23,12],[23,13],[22,14],[21,14],[20,13],[19,11],[17,6],[15,3],[13,1],[11,0],[7,0],[5,1],[4,2],[3,4],[3,6],[4,8],[5,9],[12,13],[13,14],[14,16],[14,18],[13,20],[11,21],[9,20],[8,18],[8,16],[9,13],[11,10],[16,3],[18,1],[20,0],[22,0],[23,1],[23,2]] },
    '\'': { width: 10, points: [[5,19],[4,20],[5,21],[6,20],[6,18],[5,16],[4,15]] },
    '(': { width: 14, points: [[11,25],[9,23],[7,20],[5,16],[4,11],[4,7],[5,2],[7,-2],[9,-5],[11,-7]] },
    ')': { width: 14, points: [[3,25],[5,23],[7,20],[9,16],[10,11],[10,7],[9,2],[7,-2],[5,-5],[3,-7]] },
    '*': { width: 16, points: [[8,21],[8,9],[-1,-1],[3,18],[13,12],[-1,-1],[13,18],[3,12]] },
    '+': { width: 26, points: [[13,18],[13,0],[-1,-1],[4,9],[22,9]] },
    ',': { width: 10, points: [[6,1],[5,0],[4,1],[5,2],[6,1],[6,-1],[5,-3],[4,-4]] },
    '-': { width: 26, points: [[4,9],[22,9]] },
    '.': { width: 10, points: [[5,2],[4,1],[5,0],[6,1],[5,2]] },
    '/': { width: 22, points: [[20,25],[2,-7]] },
    '0': { width: 20, points: [[9,21],[6,20],[4,17],[3,12],[3,9],[4,4],[6,1],[9,0],[11,0],[14,1],[16,4],[17,9],[17,12],[16,17],[14,20],[11,21],[9,21]] },
    '1': { width: 20, points: [[6,17],[8,18],[11,21],[11,0]] },
    '2': { width: 20, points: [[4,16],[4,17],[5,19],[6,20],[8,21],[12,21],[14,20],[15,19],[16,17],[16,15],[15,13],[13,10],[3,0],[17,0]] },
    '3': { width: 20, points: [[5,21],[16,21],[10,13],[13,13],[15,12],[16,11],[17,8],[17,6],[16,3],[14,1],[11,0],[8,0],[5,1],[4,2],[3,4]] },
    '4': { width: 20, points: [[13,21],[3,7],[18,7],[-1,-1],[13,21],[13,0]] },
    '5': { width: 20, points: [[15,21],[5,21],[4,12],[5,13],[8,14],[11,14],[14,13],[16,11],[17,8],[17,6],[16,3],[14,1],[11,0],[8,0],[5,1],[4,2],[3,4]] },
    '6': { width: 20, points: [[16,18],[15,20],[12,21],[10,21],[7,20],[5,17],[4,12],[4,7],[5,3],[7,1],[10,0],[11,0],[14,1],[16,3],[17,6],[17,7],[16,10],[14,12],[11,13],[10,13],[7,12],[5,10],[4,7]] },
    '7': { width: 20, points: [[17,21],[7,0],[-1,-1],[3,21],[17,21]] },
    '8': { width: 20, points: [[8,21],[5,20],[4,18],[4,16],[5,14],[7,13],[11,12],[14,11],[16,9],[17,7],[17,4],[16,2],[15,1],[12,0],[8,0],[5,1],[4,2],[3,4],[3,7],[4,9],[6,11],[9,12],[13,13],[15,14],[16,16],[16,18],[15,20],[12,21],[8,21]] },
    '9': { width: 20, points: [[16,14],[15,11],[13,9],[10,8],[9,8],[6,9],[4,11],[3,14],[3,15],[4,18],[6,20],[9,21],[10,21],[13,20],[15,18],[16,14],[16,9],[15,4],[13,1],[10,0],[8,0],[5,1],[4,3]] },
    ':': { width: 10, points: [[5,14],[4,13],[5,12],[6,13],[5,14],[-1,-1],[5,2],[4,1],[5,0],[6,1],[5,2]] },
    ',': { width: 10, points: [[5,14],[4,13],[5,12],[6,13],[5,14],[-1,-1],[6,1],[5,0],[4,1],[5,2],[6,1],[6,-1],[5,-3],[4,-4]] },
    '<': { width: 24, points: [[20,18],[4,9],[20,0]] },
    '=': { width: 26, points: [[4,12],[22,12],[-1,-1],[4,6],[22,6]] },
    '>': { width: 24, points: [[4,18],[20,9],[4,0]] },
    '?': { width: 18, points: [[3,16],[3,17],[4,19],[5,20],[7,21],[11,21],[13,20],[14,19],[15,17],[15,15],[14,13],[13,12],[9,10],[9,7],[-1,-1],[9,2],[8,1],[9,0],[10,1],[9,2]] },
    '@': { width: 27, points: [[18,13],[17,15],[15,16],[12,16],[10,15],[9,14],[8,11],[8,8],[9,6],[11,5],[14,5],[16,6],[17,8],[-1,-1],[12,16],[10,14],[9,11],[9,8],[10,6],[11,5],[-1,-1],[18,16],[17,8],[17,6],[19,5],[21,5],[23,7],[24,10],[24,12],[23,15],[22,17],[20,19],[18,20],[15,21],[12,21],[9,20],[7,19],[5,17],[4,15],[3,12],[3,9],[4,6],[5,4],[7,2],[9,1],[12,0],[15,0],[18,1],[20,2],[21,3],[-1,-1],[19,16],[18,8],[18,6],[19,5]] },
    'A': { width: 18, points: [[9,21],[1,0],[-1,-1],[9,21],[17,0],[-1,-1],[4,7],[14,7]] },
    'B': { width: 21, points: [[4,21],[4,0],[-1,-1],[4,21],[13,21],[16,20],[17,19],[18,17],[18,15],[17,13],[16,12],[13,11],[-1,-1],[4,11],[13,11],[16,10],[17,9],[18,7],[18,4],[17,2],[16,1],[13,0],[4,0]] },
    'C': { width: 21, points: [[18,16],[17,18],[15,20],[13,21],[9,21],[7,20],[5,18],[4,16],[3,13],[3,8],[4,5],[5,3],[7,1],[9,0],[13,0],[15,1],[17,3],[18,5]] },
    'D': { width: 21, points: [[4,21],[4,0],[-1,-1],[4,21],[11,21],[14,20],[16,18],[17,16],[18,13],[18,8],[17,5],[16,3],[14,1],[11,0],[4,0]] },
    'E': { width: 19, points: [[4,21],[4,0],[-1,-1],[4,21],[17,21],[-1,-1],[4,11],[12,11],[-1,-1],[4,0],[17,0]] },
    'F': { width: 18, points: [[4,21],[4,0],[-1,-1],[4,21],[17,21],[-1,-1],[4,11],[12,11]] },
    'G': { width: 21, points: [[18,16],[17,18],[15,20],[13,21],[9,21],[7,20],[5,18],[4,16],[3,13],[3,8],[4,5],[5,3],[7,1],[9,0],[13,0],[15,1],[17,3],[18,5],[18,8],[-1,-1],[13,8],[18,8]] },
    'H': { width: 22, points: [[4,21],[4,0],[-1,-1],[18,21],[18,0],[-1,-1],[4,11],[18,11]] },
    'I': { width: 8, points: [[4,21],[4,0]] },
    'J': { width: 16, points: [[12,21],[12,5],[11,2],[10,1],[8,0],[6,0],[4,1],[3,2],[2,5],[2,7]] },
    'K': { width: 21, points: [[4,21],[4,0],[-1,-1],[18,21],[4,7],[-1,-1],[9,12],[18,0]] },
    'L': { width: 17, points: [[4,21],[4,0],[-1,-1],[4,0],[16,0]] },
    'M': { width: 24, points: [[4,21],[4,0],[-1,-1],[4,21],[12,0],[-1,-1],[20,21],[12,0],[-1,-1],[20,21],[20,0]] },
    'N': { width: 22, points: [[4,21],[4,0],[-1,-1],[4,21],[18,0],[-1,-1],[18,21],[18,0]] },
    'O': { width: 22, points: [[9,21],[7,20],[5,18],[4,16],[3,13],[3,8],[4,5],[5,3],[7,1],[9,0],[13,0],[15,1],[17,3],[18,5],[19,8],[19,13],[18,16],[17,18],[15,20],[13,21],[9,21]] },
    'P': { width: 21, points: [[4,21],[4,0],[-1,-1],[4,21],[13,21],[16,20],[17,19],[18,17],[18,14],[17,12],[16,11],[13,10],[4,10]] },
    'Q': { width: 22, points: [[9,21],[7,20],[5,18],[4,16],[3,13],[3,8],[4,5],[5,3],[7,1],[9,0],[13,0],[15,1],[17,3],[18,5],[19,8],[19,13],[18,16],[17,18],[15,20],[13,21],[9,21],[-1,-1],[12,4],[18,-2]] },
    'R': { width: 21, points: [[4,21],[4,0],[-1,-1],[4,21],[13,21],[16,20],[17,19],[18,17],[18,15],[17,13],[16,12],[13,11],[4,11],[-1,-1],[11,11],[18,0]] },
    'S': { width: 20, points: [[17,18],[15,20],[12,21],[8,21],[5,20],[3,18],[3,16],[4,14],[5,13],[7,12],[13,10],[15,9],[16,8],[17,6],[17,3],[15,1],[12,0],[8,0],[5,1],[3,3]] },
    'T': { width: 16, points: [[8,21],[8,0],[-1,-1],[1,21],[15,21]] },
    'U': { width: 22, points: [[4,21],[4,6],[5,3],[7,1],[10,0],[12,0],[15,1],[17,3],[18,6],[18,21]] },
    'V': { width: 18, points: [[1,21],[9,0],[-1,-1],[17,21],[9,0]] },
    'W': { width: 24, points: [[2,21],[7,0],[-1,-1],[12,21],[7,0],[-1,-1],[12,21],[17,0],[-1,-1],[22,21],[17,0]] },
    'X': { width: 20, points: [[3,21],[17,0],[-1,-1],[17,21],[3,0]] },
    'Y': { width: 18, points: [[1,21],[9,11],[9,0],[-1,-1],[17,21],[9,11]] },
    'Z': { width: 20, points: [[17,21],[3,0],[-1,-1],[3,21],[17,21],[-1,-1],[3,0],[17,0]] },
    '[': { width: 14, points: [[4,25],[4,-7],[-1,-1],[5,25],[5,-7],[-1,-1],[4,25],[11,25],[-1,-1],[4,-7],[11,-7]] },
    '\\': { width: 14, points: [[0,21],[14,-3]] },
    ']': { width: 14, points: [[9,25],[9,-7],[-1,-1],[10,25],[10,-7],[-1,-1],[3,25],[10,25],[-1,-1],[3,-7],[10,-7]] },
    '^': { width: 16, points: [[6,15],[8,18],[10,15],[-1,-1],[3,12],[8,17],[13,12],[-1,-1],[8,17],[8,0]] },
    '_': { width: 16, points: [[0,-2],[16,-2]] },
    '`': { width: 10, points: [[6,21],[5,20],[4,18],[4,16],[5,15],[6,16],[5,17]] },
    'a': { width: 19, points: [[15,14],[15,0],[-1,-1],[15,11],[13,13],[11,14],[8,14],[6,13],[4,11],[3,8],[3,6],[4,3],[6,1],[8,0],[11,0],[13,1],[15,3]] },
    'b': { width: 19, points: [[4,21],[4,0],[-1,-1],[4,11],[6,13],[8,14],[11,14],[13,13],[15,11],[16,8],[16,6],[15,3],[13,1],[11,0],[8,0],[6,1],[4,3]] },
    'c': { width: 18, points: [[15,11],[13,13],[11,14],[8,14],[6,13],[4,11],[3,8],[3,6],[4,3],[6,1],[8,0],[11,0],[13,1],[15,3]] },
    'd': { width: 19, points: [[15,21],[15,0],[-1,-1],[15,11],[13,13],[11,14],[8,14],[6,13],[4,11],[3,8],[3,6],[4,3],[6,1],[8,0],[11,0],[13,1],[15,3]] },
    'e': { width: 18, points: [[3,8],[15,8],[15,10],[14,12],[13,13],[11,14],[8,14],[6,13],[4,11],[3,8],[3,6],[4,3],[6,1],[8,0],[11,0],[13,1],[15,3]] },
    'f': { width: 12, points: [[10,21],[8,21],[6,20],[5,17],[5,0],[-1,-1],[2,14],[9,14]] },
    'g': { width: 19, points: [[15,14],[15,-2],[14,-5],[13,-6],[11,-7],[8,-7],[6,-6],[-1,-1],[15,11],[13,13],[11,14],[8,14],[6,13],[4,11],[3,8],[3,6],[4,3],[6,1],[8,0],[11,0],[13,1],[15,3]] },
    'h': { width: 19, points: [[4,21],[4,0],[-1,-1],[4,10],[7,13],[9,14],[12,14],[14,13],[15,10],[15,0]] },
    'i': { width: 8, points: [[3,21],[4,20],[5,21],[4,22],[3,21],[-1,-1],[4,14],[4,0]] },
    'j': { width: 10, points: [[5,21],[6,20],[7,21],[6,22],[5,21],[-1,-1],[6,14],[6,-3],[5,-6],[3,-7],[1,-7]] },
    'k': { width: 17, points: [[4,21],[4,0],[-1,-1],[14,14],[4,4],[-1,-1],[8,8],[15,0]] },
    'l': { width: 8, points: [[4,21],[4,0]] },
    'm': { width: 30, points: [[4,14],[4,0],[-1,-1],[4,10],[7,13],[9,14],[12,14],[14,13],[15,10],[15,0],[-1,-1],[15,10],[18,13],[20,14],[23,14],[25,13],[26,10],[26,0]] },
    'n': { width: 19, points: [[4,14],[4,0],[-1,-1],[4,10],[7,13],[9,14],[12,14],[14,13],[15,10],[15,0]] },
    'o': { width: 19, points: [[8,14],[6,13],[4,11],[3,8],[3,6],[4,3],[6,1],[8,0],[11,0],[13,1],[15,3],[16,6],[16,8],[15,11],[13,13],[11,14],[8,14]] },
    'p': { width: 19, points: [[4,14],[4,-7],[-1,-1],[4,11],[6,13],[8,14],[11,14],[13,13],[15,11],[16,8],[16,6],[15,3],[13,1],[11,0],[8,0],[6,1],[4,3]] },
    'q': { width: 19, points: [[15,14],[15,-7],[-1,-1],[15,11],[13,13],[11,14],[8,14],[6,13],[4,11],[3,8],[3,6],[4,3],[6,1],[8,0],[11,0],[13,1],[15,3]] },
    'r': { width: 13, points: [[4,14],[4,0],[-1,-1],[4,8],[5,11],[7,13],[9,14],[12,14]] },
    's': { width: 17, points: [[14,11],[13,13],[10,14],[7,14],[4,13],[3,11],[4,9],[6,8],[11,7],[13,6],[14,4],[14,3],[13,1],[10,0],[7,0],[4,1],[3,3]] },
    't': { width: 12, points: [[5,21],[5,4],[6,1],[8,0],[10,0],[-1,-1],[2,14],[9,14]] },
    'u': { width: 19, points: [[4,14],[4,4],[5,1],[7,0],[10,0],[12,1],[15,4],[-1,-1],[15,14],[15,0]] },
    'v': { width: 16, points: [[2,14],[8,0],[-1,-1],[14,14],[8,0]] },
    'w': { width: 22, points: [[3,14],[7,0],[-1,-1],[11,14],[7,0],[-1,-1],[11,14],[15,0],[-1,-1],[19,14],[15,0]] },
    'x': { width: 17, points: [[3,14],[14,0],[-1,-1],[14,14],[3,0]] },
    'y': { width: 16, points: [[2,14],[8,0],[-1,-1],[14,14],[8,0],[6,-4],[4,-6],[2,-7],[1,-7]] },
    'z': { width: 17, points: [[14,14],[3,0],[-1,-1],[3,14],[14,14],[-1,-1],[3,0],[14,0]] },
    '{': { width: 14, points: [[9,25],[7,24],[6,23],[5,21],[5,19],[6,17],[7,16],[8,14],[8,12],[6,10],[-1,-1],[7,24],[6,22],[6,20],[7,18],[8,17],[9,15],[9,13],[8,11],[4,9],[8,7],[9,5],[9,3],[8,1],[7,0],[6,-2],[6,-4],[7,-6],[-1,-1],[6,8],[8,6],[8,4],[7,2],[6,1],[5,-1],[5,-3],[6,-5],[7,-6],[9,-7]] },
    '|': { width: 8, points: [[4,25],[4,-7]] },
    '}': { width: 14, points: [[5,25],[7,24],[8,23],[9,21],[9,19],[8,17],[7,16],[6,14],[6,12],[8,10],[-1,-1],[7,24],[8,22],[8,20],[7,18],[6,17],[5,15],[5,13],[6,11],[10,9],[6,7],[5,5],[5,3],[6,1],[7,0],[8,-2],[8,-4],[7,-6],[-1,-1],[8,8],[6,6],[6,4],[7,2],[8,1],[9,-1],[9,-3],[8,-5],[7,-6],[5,-7]] },
    '~': { width: 24, points: [[3,6],[3,8],[4,11],[6,12],[8,12],[10,11],[14,8],[16,7],[18,7],[20,8],[21,10],[-1,-1],[3,8],[4,10],[6,11],[8,11],[10,10],[14,7],[16,6],[18,6],[20,7],[21,10],[21,12]] }
};

CanvasTextFunctions.letter = function (ch)
{
    return CanvasTextFunctions.letters[ch];
}

CanvasTextFunctions.ascent = function( font, size)
{
    return size;
}

CanvasTextFunctions.descent = function( font, size)
{
    return 7.0*size/25.0;
}

CanvasTextFunctions.measure = function( font, size, str)
{
    var total = 0;
    var len = str.length;

    for ( i = 0; i < len; i++) {
		var c = CanvasTextFunctions.letter( str.charAt(i));
		if ( c) total += c.width * size / 25.0;
    }
    return total;
}

CanvasTextFunctions.draw = function(ctx,font,size,x,y,str)
{
    var total = 0;
    var len = str.length;
    var mag = size / 25.0;

    ctx.save();
    ctx.lineCap = "round";
    ctx.lineWidth = 2.0 * mag;
	ctx.strokeStyle = ctx.fillStyle;

    for ( i = 0; i < len; i++) {
		var c = CanvasTextFunctions.letter( str.charAt(i));
		if ( !c)
			continue;

		ctx.beginPath();

		var penUp = 1;
		var needStroke = 0;
		for ( j = 0; j < c.points.length; j++) {
		    var a = c.points[j];
		    if ( a[0] == -1 && a[1] == -1) {
				penUp = 1;
				continue;
		    }
		    if ( penUp) {
				ctx.moveTo( x + a[0]*mag, y - a[1]*mag);
				penUp = false;
		    } else {
				ctx.lineTo( x + a[0]*mag, y - a[1]*mag);
		    }
		}
		ctx.stroke();
		x += c.width*mag;
    }
    ctx.restore();
    return total;
}

CanvasTextFunctions.enable = function( ctx)
{
    ctx.drawText = function(font,size,x,y,text) { return CanvasTextFunctions.draw( ctx, font,size,x,y,text); };
    ctx.measureText = function(font,size,text) { return CanvasTextFunctions.measure( font,size,text); };
    ctx.fontAscent = function(font,size) { return CanvasTextFunctions.ascent(font,size); }
    ctx.fontDescent = function(font,size) { return CanvasTextFunctions.descent(font,size); }

    ctx.drawTextRight = function(font,size,x,y,text) {
		var w = CanvasTextFunctions.measure(font,size,text);
		return CanvasTextFunctions.draw( ctx, font,size,x-w,y,text);
    };
    ctx.drawTextCenter = function(font,size,x,y,text) {
		var w = CanvasTextFunctions.measure(font,size,text);
		return CanvasTextFunctions.draw( ctx, font,size,x-w/2,y,text);
    };
}

/***************************************************
 * EPLOT class
 ***************************************************/

function ev_mouse_down( ev )
{
	p.onMouseDown(ev);
}

function ev_mouse_up( ev )
{
	if (p.onMouseUp(ev))
		btn_set_xpos();
}

function ev_mouse_move( ev )
{
	if (p == undefined)
		return;
	var xrange = p.onMouseMove(ev);
	if (!xrange)
		return;
}

function ev_key_down( ev )
{
	p.onKeyDown(ev);
}

function eStaple(name, clr1, clr2, width, pts)
{
	this.clr = [];
	this.name = name;
	this.clr[0] = clr1;
	this.clr[1] = clr2;
	this.width = width;
	this.pts = pts;
}

function ePlot(id, width, height)
{
	if (navigator.appVersion.indexOf("MSIE") != -1) {
		var ver = navigator.appVersion.split("MSIE")[1];
		if (ver < 8)
			alert('This page can\'t be showed correctly in IExplorer < 8');
		else
			alert('This page can be slow in IExplorer, use another browser');
	}

	if (navigator.userAgent.indexOf("Opera") != -1)
		alert('Some features may not work correctly in Opera due to scroll overflow bug. Use firefox/safari/chrome');

	var root = document.getElementById(id);

	// cnv[0] - main plot
	// cnv[1] - Y axis
	// cnv[2] - selection rectangle
	var cnv = [];
	var ctx = [];

	for (var i = 0; i < 3; i++) {
		cnv[i] = document.createElement('canvas');

		cnv[i].style.overflow = 'auto';
		cnv[i].style.position = 'absolute';
		cnv[i].style.left = 0;
		cnv[i].style.top = 0;
		root.appendChild(cnv[i]);
		if (G_vmlCanvasManager != undefined)
			G_vmlCanvasManager.initElement(cnv[i]);

		if (width != undefined)
			cnv[i].width = width;
		if (height != undefined)
			cnv[i].height = height;
		ctx[i] = cnv[i].getContext('2d');
		if (ctx[i].fillText == undefined)
			CanvasTextFunctions.enable(ctx[i]);
	}
	cnv[0].style.position = 'relative';

	// Init mouse handlers on 'selection' canvas
	cnv[2].onmousedown = ev_mouse_down;
	cnv[2].onmouseup = ev_mouse_up;
	cnv[2].onmousemove = ev_mouse_move;
	ctx[2].globalAlpha = 0.5;
	ctx[2].fillStyle = '#c000c0';
	ctx[2].lineWidth = 1;
	ctx[2].textBaselign = 'bottom';
	ctx[2].textAlign = 'left';
	document.onkeydown = ev_key_down;

	this.ctx = ctx;
	this.cnv = cnv;

	this.log = [];

	this.fontH = 12;
	this.ctx.font = '9pt Verdana, Arial';
	this.stat_total_updated = 0;
	this.y_axis_labels_drawn = 0;
	this.y_axis_title_drawn = 0;

	this.x_marker = 0;
	this.do_log_scroll = 1;

	this.find_pos = function( ev, obj )
	{
		var left, top;

		if ( !ev )
			ev = window.event;

		if (obj.offsetParent) {
			left = obj.offsetLeft;
			top  = obj.offsetTop;
			while (obj = obj.offsetParent) {
				left += obj.offsetLeft;
				top  += obj.offsetTop;
			}
		}
		if (ev.clientX != undefined)
			return [ev.clientX - left, ev.clientTop - top];
		else
			return [ev.offsetX - left, ev.offsetTop - top];
	}

	this.mouse_down = 0;
	this.mouse_pos_down = [0, 0];
	this.mouse_pos_move = [0, 0];

	this.saveXminXmaxVis = function ()
	{
		if (this.saved_xmin_vis == -1)
			this.saved_xmin_vis = document.getElementById('text_xmin_vis').value;
		if (this.saved_xmax_vis == -1)
			this.saved_xmax_vis = document.getElementById('text_xmax_vis').value;
	}

	this.restoreSavedXminXmaxVis = function ()
	{
		document.getElementById('text_xmin_vis').value = this.saved_xmin_vis;
		document.getElementById('text_xmax_vis').value = this.saved_xmax_vis;
	}

	this.resetSavedXminXmaxVis = function ()
	{
		this.saved_xmin_vis = -1;
		this.saved_xmax_vis = -1;
	}
	this.resetSavedXminXmaxVis();

	this.cancelSelection = function ()
	{
		if (!this.mouse_down)
			return;

		this.restoreSavedXminXmaxVis();
		this.resetSavedXminXmaxVis();

		this.mouse_down = 0;
		this.drawMarker();
	}

	this.isRightButton = function ( ev )
	{
		if ( !ev )
			ev = window.event;

		if (ev.which == undefined) {
			// IE
			if (ev.button & 2)
				return 1;
		} else {
			// Opera, Firefox, Safari
			if (ev.which == 3)
				return 1;
		}
		return 0;
	}

	this.onKeyDown = function ( ev )
	{
		if ( !ev )
			ev = window.event;

		var key = ev.keyCode ? ev.keyCode :
			ev.charCode ? ev.charCode :
			ev.which ? ev.which : void 0;

		switch (key) {
		case 27: // escape
			this.cancelSelection();
			break;
		}
	}

	this.onMouseDown = function( ev )
	{
		if ( this.isRightButton(ev) ) {
			this.cancelSelection();
			return false;
		}

		this.mouse_pos_down = this.find_pos(ev, p.cnv[0]);
		this.mouse_down = 1;
		this.x_marker = this.plot2x(this.mouse_pos_down[0]);
		this.scrollLog();
		this.drawMarker();
	}

	this.parseLogTime = function( n )
	{
		if (!this.log.length)
			this.log = document.getElementById('log').innerHTML.split("\n");

		var t = parseFloat(this.log[n]);

		if (t == 0 || isNaN(t)) {
			/* Probably there are some HTML tags? */
			this.log[n] = this.log[n].replace(/<\/?[^>]+(>|$)/g, "");
			t = parseFloat(this.log[n]);
		}
		return t;
	}

	this.scrollLog = function ( )
	{
		if (!this.log.length)
			this.log = document.getElementById('log').innerHTML.split("\n");

		var min = 0;
		var max = this.log.length;

		while (Math.round(min) != Math.round(max) - 1) {
			var n = (max - min) / 2 + min;
			var __x = this.parseLogTime(Math.round(n));

			if (this.x_marker < __x)
				max = n;
			else if (this.x_marker > __x)
				min = n;
			else {
				min = n;
				break;
			}
		}

		var id = document.getElementById('log');

		this.do_log_scroll = 0;
		id.scrollTop = id.scrollHeight * (min / this.log.length);
	}

	this.onMouseUp = function( ev )
	{
		if (!this.mouse_down)
			return false;
		if (this.isRightButton(ev))
			return false;
		this.mouse_down = 0;
		this.ctx[2].clearRect(0, 0, this.cnv[2].width, this.cnv[2].height);
		this.drawMarker();

		/* Don't zoom if selected range is < 5 pixels */
		var delta = Math.abs(this.mouse_pos_down[0] - this.mouse_pos_move[0]);
		if (delta < 5)
			return false;

		return true;
	}

	this.onMouseMove = function ( ev )
	{
		ctx = this.ctx[2];

		if (!this.mouse_down)
			return;

		if (this.isRightButton(ev))
			return;

		this.mouse_pos_move = this.find_pos(ev, p.cnv[0]);
		ctx.clearRect(0, 0, this.cnv[2].width, this.cnv[2].height);

		var x_plot = Math.min(this.mouse_pos_move[0], this.mouse_pos_down[0]);
		var w = Math.abs(this.mouse_pos_move[0] - this.mouse_pos_down[0]);

		if (w == 0)
			w = 1;

		ctx.fillRect(x_plot, this.pt, w, this.pt + this.h);

		this.fillText(ctx, 'delta: ' +
			Math.round((this.plot2x(x_plot + w) - this.plot2x(x_plot)) * 1000) / 1000,
			Math.round(x_plot + w) + 1, this.pt);

		var x = this.plot2x(x_plot);

		if (x != this.x_marker) {
			this.x_marker = x;
			this.scrollLog();
		}

		if (document.getElementById('text_xmin_vis') == undefined)
			return;
		if (document.getElementById('text_xmax_vis') == undefined)
			return;

		this.saveXminXmaxVis();

		document.getElementById('text_xmin_vis').value = Math.round(x * 10) / 10;
		document.getElementById('text_xmax_vis').value = Math.round(this.plot2x(x_plot + w) * 10) / 10;
	}

	this.onLogScroll = function ()
	{
		if (!this.do_log_scroll) {
			this.do_log_scroll = 1;
			return;
		}

		if (!this.log.length)
			this.log = document.getElementById('log').innerHTML.split("\n");

		var id = document.getElementById('log');
		var n = this.log.length * id.scrollTop / id.scrollHeight;
		var x = this.parseLogTime(Math.round(n));

		this.x_marker = x;
		this.drawMarker();
	}

	this.drawMarker = function ( )
	{
		var ctx = this.ctx[2];

		var x = this.x_marker;

		ctx.clearRect(0, 0, this.cnv[2].width, this.cnv[2].height);
		if (x < this.x_min || x > this.x_max)
			return;

		ctx.beginPath();

		ctx.moveTo(this.getx(x), this.gety(this.y_min));
		ctx.lineTo(this.getx(x), this.gety(this.y_max));

		this.fillText(ctx, Math.round(x * 1000) / 1000, this.getx(x) + 0.5, this.pt);

		ctx.stroke();
	}

	this.fillText = function (ctx, text, x, y)
	{
		if (ctx.fillText != undefined)
			return ctx.fillText(text, x, y);

		ctx.stroke();

		if (ctx.textAlign == 'center')
			ctx.drawTextCenter(ctx.font, 9, x, y, text);
		else if (ctx.textAlign == 'right')
			ctx.drawTextRight(ctx.font, 9, x, y, text);
		else
			ctx.drawText(ctx.font, 9, x, y, text);
	}

	this.lnX = 0;
	this.lnY = 0;
	this.staples = [];

	this.setPadding = function(left, right, top, bottom)
	{
		this.w = this.cnv[0].width - left - right;
		this.h = this.cnv[0].height - top - bottom;
		this.pl = left;
		this.pt = top;
	}

	this.x2ln = function (val)
	{
		if (val == 0)
			return 0;
		if (val > 0)
			return Math.log(val / this.lnX);
		return -Math.log(-val/this.lnX);
	}

	this.y2ln = function (val)
	{
		if (val == 0)
			return 0;
		if (val > 0)
			return Math.log(val / this.lnY);
		return -Math.log(-val/this.lnY);
	}

	this.plot2x = function (plot_x)
	{
		if (this.lnX) {
			alert('plot2x is not implemented for logarithm X scale');
			return 0;
		}

		var x = this.x_min + (this.x_max - this.x_min) * (plot_x - this.pl) / this.w;

		return x;
	}

	this.getx = function (x)
	{
		var newx;
		if (this.lnX == 0) {
			newx = (x - this.x_min) / (this.x_max - this.x_min);
		} else {
			newx = (this.x2ln(x) - this.x_min_ln) / (this.x_max_ln - this.x_min_ln);
		}
		return 0.5 + Math.round(this.pl + this.w * newx);
	}

	this.gety = function (y) {
		var newy;
		if (this.lnY == 0) {
			newy = (y - this.y_min) / (this.y_max - this.y_min);
		} else {
			newy = (this.y2ln(y) - this.y_min_ln) / (this.y_max_ln - this.y_min_ln);
		}
		return 0.5 + Math.round(this.pt + this.h - this.h * newy);
	}

	this.setLnScale = function (lnX, lnY)
	{
		this.lnX = lnX;
		this.lnY = lnY;

		this.x_min_ln = this.x2ln(this.x_min);
		this.x_max_ln = this.x2ln(this.x_max);

		this.y_min_ln = this.y2ln(this.y_min);
		this.y_max_ln = this.y2ln(this.y_max);
	}

	this.setRangeX = function (min, max)
	{
		this.x_min = min;
		this.x_max = max;

		this.setLnScale(this.lnX, this.lnY);
	}

	this.setRangeY = function (min, max)
	{
		this.y_min = min;
		this.y_max = max;

		this.setLnScale(this.lnX, this.lnY);
	}

	this.setPadding(30, 0, 0, 30);
	this.setRangeX(0, 100);
	this.setRangeY(0, 100);

	this.drawAxis = function ()
	{
		var ctx = this.ctx[0];

		ctx.strokeStyle = '#808080';
		ctx.fillStyle = '#808080';
		ctx.lineWidth = 1;
		ctx.lineCap = "round";
		ctx.beginPath();

		if (this.x_min < 0 && 0 < this.x_max)
		{
			ctx.moveTo(this.getx(0), this.gety(this.y_min));
			ctx.lineTo(this.getx(0), this.gety(this.y_max));
		}

		if (this.y_min < 0 && 0 < this.y_max)
		{
			ctx.moveTo(this.getx(this.x_min), this.gety(0));
			ctx.lineTo(this.getx(this.x_max), this.gety(0));
		}
		ctx.stroke();
	}

	this.setAxisTitle = function (xname, yname)
	{
		this.xname = xname;
		this.yname = yname;
	}

	this.drawAxisTitle = function ()
	{
		var ctx = this.ctx[0];

		if (this.xname != undefined) {
			ctx.textAlign = 'center';
			this.fillText(ctx, this.xname, Math.round(this.w / 2),
				Math.round(this.h + this.pt + 3 * this.fontH));
		}
		if (this.yname != undefined && !this.y_axis_title_drawn) {
			var h = this.fontH;
			ctx = this.ctx[1];

			ctx.textBaselign = 'top';
			ctx.textAlign = 'center';
			var y = Math.round(this.h / 2);
			y -= Math.round((this.yname.length - 2) * h / 2);
			for (var i = 0; i < this.yname.length; i++) {
				this.fillText(ctx, this.yname.charAt(i), 6, y);
				y += h;
			}

			this.y_axis_title_drawn = 1;
		}
	}

	this.setGrid = function (xstep, ystep, type)
	{
		if (type == undefined)
			type = 'normal';

		var xgrid = [];
		var ygrid = [];

		if (xstep.constructor == Array) {
			if (xstep[0].constructor == Array)
				xgrid = xstep;
			else
				for (var i = 0; i < xstep.length; i++)
					xgrid[i] = [xstep[i], xstep[i]];
		} else {
			var xstart = Math.ceil(this.x_min / xstep) * xstep;
			var x = xstart;
			for (var i = 0; x <= this.x_max; i++, x += xstep)
				xgrid[i] = [x, Math.round(x * 100) / 100];
		}

		if (ystep.constructor == Array) {
			if (ystep[0].constructor == Array)
				ygrid = ystep;
			else
				for (var i = 0; i < ystep.length; i++)
					ygrid[i] = [ystep[i], ystep[i]];
		} else {
			var ystart = Math.ceil(this.y_min / ystep) * ystep;

			var y = ystart;
			for (var i = 0; y < this.y_max; i++, y += ystep)
				ygrid[i] = [y, Math.round(y * 10) / 10];
		}

		this.xgrid = xgrid;
		this.ygrid = ygrid;
	}

	this.setStaples = function (name, clr1, clr2, width, pts)
	{
		this.staples[name] = new eStaple(name, clr1, clr2, width, pts);
	}

	this.stat_reset = function(time)
	{
		/* st_ - stats */
		this.st_wait = [];		/* wait time */
		this.st_req = [];		/* request count (total) */
		this.st_size = [];		/* bytes total */
		this.st_size_bef = [];	/* bytes total read/written before */
		this.st_time = [];		/* time interval */
		this.st_aligned = [];	/* page aligned */
		this.st_grp = [];		/* request count (in groups by size) */
		this.st_seq = [];		/* number of sequential requests */
		this.st_prev_addr = [];

		for (var t = 0; t < 2; t++) {
			this.st_wait[t] = this.st_req[t] = this.st_size[t] = 0;
			this.st_size_bef[t] = this.st_aligned[t] = 0;
			this.st_time[t] = time;

			this.st_grp[t] = [];
			this.st_grp[t][0] = 0;
			this.st_grp[t][1] = 0;
			this.st_grp[t][2] = 0;
			this.st_seq[t] = 0;
			this.st_prev_addr[t] = -1;
		}
	}

	this.stat_add = function(t, x1, x2, y, addr)
	{
		var abs_y = Math.abs(y);
		this.st_wait[t] += x2 - x1;
		this.st_size[t] += abs_y;
		this.st_req[t]++;

		if (!(y % 4096))
			this.st_aligned[t]++;

		if (abs_y <= 4096)
			this.st_grp[t][0]++;
		else if (abs_y < 1024 * 1024)
			this.st_grp[t][1]++;
		else
			this.st_grp[t][2]++;
		if (addr < 0)
			return;

		// alert('addr: ' + addr + ' + len: ' + abs_y + ' = ' + (addr + abs_y) + ', prev: ' + this.st_prev_addr[t]);
		if (this.st_prev_addr[t] == -1 || this.st_prev_addr[t] == addr)
			this.st_seq[t]++;
		this.st_prev_addr[t] = addr + abs_y;
	}

	this.stat_update = function()
	{
		var max = 2;
		if (this.stat_total_updated == 0) {
			this.stat_total_updated = 1;
			max = 4;
		}
		for (var n = 0; n < max; n++) {
			var t = n % 2;

			document.getElementById('st_time_' + n).innerHTML	= Math.ceil(this.st_time[t]);
			document.getElementById('st_req_' + n).innerHTML		= this.st_req[t];

			if (this.st_time[t])
				document.getElementById('st_wait_' + n).innerHTML	= Math.ceil(this.st_wait[t]) + " (" + Math.ceil(this.st_wait[t] * 1000 / this.st_time[t]) / 10 + "%)";
			else
				document.getElementById('st_wait_' + n).innerHTML	= 0;

			var mb = Math.ceil(this.st_size[t] / (1024 * 10.24)) / 100;
			if (n < 2)
				document.getElementById('st_mb_' + n).innerHTML	= mb + " / " + Math.ceil(this.st_size_bef[t] / (1024 * 102.4)) / 10;
			else
				document.getElementById('st_mb_' + n).innerHTML	= mb;

			if (this.st_req[t]) {
				document.getElementById('st_aligned_' + n).innerHTML	= this.st_aligned[t] + ' (' +
					Math.ceil(this.st_aligned[t] * 1000 / this.st_req[t]) / 10 + '%)';
				document.getElementById('st_avg_' + n).innerHTML		= Math.ceil((100 * this.st_size[t] / 1024.0) / this.st_req[t]) / 100;
				document.getElementById('st_seq_' + n).innerHTML =
					this.st_seq[t] + ' (' + Math.ceil(this.st_seq[t] * 1000 / this.st_req[t]) / 10 + '%)';
			} else {
				document.getElementById('st_aligned_' + n).innerHTML	= '0 (0%)';
				document.getElementById('st_avg_' + n).innerHTML		= '0';
				document.getElementById('st_seq_' + n).innerHTML		= '0 (0%)';
			}

			document.getElementById('st_grp0_' + n).innerHTML	= this.st_grp[t][0];
			document.getElementById('st_grp1_' + n).innerHTML	= this.st_grp[t][1];
			document.getElementById('st_grp2_' + n).innerHTML	= this.st_grp[t][2];

			var wait = this.st_wait[t] / 1000.0;
			if (wait == 0) {
				document.getElementById('st_req_s_' + n).innerHTML	= 0;
				document.getElementById('st_mb_s_' + n).innerHTML	= 0;
			} else {
				document.getElementById('st_req_s_' + n).innerHTML	= Math.ceil(100 * this.st_req[t] / wait) / 100;
				document.getElementById('st_mb_s_' + n).innerHTML	= Math.ceil(100 * this.st_size[t] / (1024 * 1024.0) / wait) / 100;
			}
		}
	}

	this.drawStaples = function (name)
	{
		var ctx = this.ctx[0];

		var st = this.staples[name];

		ctx.lineWidth = parseInt(st.width);

		var x1, y, x2, addr;

		addr = -1;

		/* st_ - stats */
		this.stat_reset(this.x_max - this.x_min);

		var ar = st.pts;
		var len = st.pts.length;

		var stat_need_update = 0;
		if (name == "plot_vm_hdd")
			stat_need_update = 1;

		var plot_x1 = 0;
		var plot_y = 0;
		var plot_x2 = 0;
		var plot_y0 = this.gety(0);
		var plot_span_skipped = 0;
		var plot_span_total = 0;

		for (var t = 0; t < 2; t++) {

			ctx.beginPath();
			ctx.strokeStyle = st.clr[t];

			/*
			 * performance optimization for wide scale:
			 * do not redraw filled areas:
			 * - if it's just a vertical span;
			 * - it was already drawn on the same x and y range
			 */
			var plot_prev_span_y_max = this.gety(this.y_min);
			var plot_prev_span_y_min = this.gety(this.y_max);
			var plot_prev_span_x = 0;

			for (var i = 0; i < len; i++) {

				y = ar[i][1];

				if (t == 0 && y > 0)
						continue;
				if (t != 0 && y < 0)
						continue;

				x1 = ar[i][0];
				x2 = ar[i][0] + ar[i][2];

				if (ar[i].length > 3)
					addr = ar[i][3];

				if (x2 < this.x_min) {
					this.st_size_bef[t] += Math.abs(y);
					continue;
				}
				if (x1 > this.x_max)
					continue;

				if (stat_need_update)
					this.stat_add(t, Math.max(x1, this.x_min), Math.min(x2, this.x_max), y, addr);

				plot_x1 = this.getx(x1);
				plot_x2 = this.getx(x2);
				plot_y = this.gety(y);

				plot_span_total++;

				if (plot_x1 == plot_x2) {
					if (plot_prev_span_x == plot_x1 &&
								plot_y <= plot_prev_span_y_max &&
								plot_y >= plot_prev_span_y_min) {
						/* ok, we are lucky */
						plot_span_skipped++;
						continue;
					} else {
						plot_prev_span_x = plot_x1;
						if (plot_y < plot_prev_span_y_min)
							plot_prev_span_y_min = plot_y;
						if (plot_y > plot_prev_span_y_max)
							plot_prev_span_y_max = plot_y;
					}
				} else {
					/* reset prev plot span */
					plot_prev_span_y_max = this.gety(this.y_min);
					plot_prev_span_y_min = this.gety(this.y_max);
					plot_prev_span_x = 0;
				}

				// Draw staple

				if (x1 < this.x_min) {
					ctx.moveTo(this.getx(this.x_min), plot_y);
				} else {
					ctx.moveTo(plot_x1, plot_y0);
					ctx.lineTo(plot_x1, plot_y);
				}

				if (plot_x2 != plot_x1) {
					if (x2 > this.x_max) {
						ctx.lineTo(this.getx(this.x_max), plot_y);
						ctx.moveTo(plot_x2, plot_y0);
					} else {
						ctx.lineTo(plot_x2, plot_y);
						ctx.lineTo(plot_x2, plot_y0);
					}
				}
				ctx.moveTo(plot_x1, plot_y0);
			}
			ctx.stroke();
		}
		if (stat_need_update)
			this.stat_update();
		// alert("Skip " + plot_span_skipped + "/" + plot_span_total);
	}

	this.setGridStyle = function(ctx)
	{
		ctx.strokeStyle = '#e0e0e0';
		ctx.fillStyle = '#808080';
		ctx.lineWidth = 1;
	   	ctx.lineCap = "round";
	}

	this.drawGrid = function ()
	{
		var ctx = this.ctx[0];

		ctx.beginPath();
		ctx.textBaselign = 'top';

		var xgrid = this.xgrid;
		var ygrid = this.ygrid;

		this.setGridStyle(ctx);

		if (!this.y_axis_labels_drawn) {
			this.ctx[1].beginPath();
			this.setGridStyle(this.ctx[1]);
			this.ctx[1].textAlign = 'right';
		}

		for (var i = 0; i < ygrid.length; i++) {

			var y = ygrid[i][0];

			if (y < this.y_min || y > this.y_max)
				continue;

			ctx.moveTo(this.getx(this.x_min), this.gety(y));
			ctx.lineTo(this.getx(this.x_max), this.gety(y));

			if (y == 0)
				continue;

			if (!this.y_axis_labels_drawn)
				this.fillText(this.ctx[1], ygrid[i][1],
					this.getx(this.x_min) - 5 + 0.5, this.gety(y) + 0.5);
		}
		if (!this.y_axis_labels_drawn) {
			this.ctx[1].stroke();
			this.y_axis_labels_drawn = 1;
		}
		ctx.stroke();

		ctx.strokeStyle = '#e0e0e0';
		ctx.fillStyle = '#808080';
		ctx.lineWidth = 1;
	   	ctx.lineCap = "round";

		ctx.beginPath();
		ctx.textAlign = 'center';

		for (var i = 0; i < xgrid.length; i++) {

			var x = xgrid[i][0];

			if (x < this.x_min || x > this.x_max)
				continue;

			ctx.moveTo(this.getx(x), this.gety(this.y_min));
			ctx.lineTo(this.getx(x), this.gety(this.y_max));

			// FIXME: add ' ' to convert float to str and fix glitch
			// in drawText() in Opera/Konqueror
			this.fillText(this.ctx[0], ' ' + xgrid[i][1] + ' ',
					this.getx(x) + 0.5, this.gety(this.y_min) + this.fontH + 0.5);
		}

		ctx.stroke();
	}

	this.clear = function ()
	{
		this.ctx[0].clearRect(0, 0, this.cnv[0].width, this.cnv[0].height);
	}

	this.draw = function ()
	{
		this.clear();
		this.w
		this.drawGrid();
		this.drawAxis();
		this.drawAxisTitle();
		for (s in this.staples) {
			if (document.getElementById('cb_' + s) != null) {
				if (document.getElementById('cb_' + s).checked == false)
					continue;
			}
			this.drawStaples(s);
		}
		this.drawMarker();
		this.resetSavedXminXmaxVis();
		this.saveXminXmaxVis();
	}
}

</script>
</head>
<script type="text/javascript">

var p;

var ymax_vis = {{REQ_SIZE}};
var ymin_vis = -ymax_vis;

var xmin = {{XMIN}};
var xmax = {{XMAX}};

var ymin = ymin_vis;
var ymax = ymax_vis;
var xmin_vis = xmin;
var xmax_vis = xmax;

var xscale_width = 300;
var xscale_max = xmax - xmin;
var xscale_min = 1;

var xscale_step = 1.01;

var padding = 5;

var xpos_width;

var show_host_hdd = {{SHOW_HOST_HDD}};
var show_vm_pcache = {{SHOW_VM_PCACHE}};

var plot_height = {{PLOT_HEIGHT}};
var plot_width = 1024;

if (typeof( window.innerWidth) == 'number')
	plot_width = window.innerWidth - padding * 8;
else if( document.body && ( document.body.clientWidth))
	plot_width = document.body.clientWidth - padding * 8;
else
	plot_width = screen.width - padding * 8;

if (screen.height > 1100)
	plot_height += screen.height - 1100

var ygrid = [];

var xpos_fill_width;
var xscale_fill_width;

var xpos;
var xpos_fill;
var xscale;
var xscale_fill;

var is_opera = (navigator.userAgent.indexOf("Opera") != -1);

function init()
{
	p = new ePlot('cnv', plot_width, plot_height);

	p.setPadding(60, 20, 10, 50);
	p.setLnScale(0, 256);

	var j = 0;
	for (var i = 512; i < (2 * ymax); i *= 2, j += 2) {
		var txt = '';
		if (i < 1024)
			txt = i;
		else if (i < 1024 * 1024)
			txt = Math.round(i / 1024) + ' K';
		else
			txt = Math.round(i / (1024 * 1024)) + ' M';

		ygrid[j] = [i, txt];
		ygrid[j + 1] = [-i, txt];
	}

	p.setRangeX(xmin, xmax);
	p.setRangeY(ymin, ymax);
	p.setGrid(1000, ygrid);

	p.setStaples('plot_vm_hdd', 'red', 'green', 2,
		[
			{{PLOT_DATA_VM_HDD}}
		]);

	if (show_vm_pcache) {
		var id = document.getElementById('td_show_vm_pcache');
		if (id)
			id.style.display = 'block';
		p.setStaples('plot_vm_pcache', 'yellowgreen', 'yellowgreen', 2,
			[
				{{PLOT_DATA_VM_PCACHE}}
			]);
	}

	p.setStaples('plot_vm_vcpu', 'blue', 'blue', 2,
		[
			{{PLOT_DATA_VM_VCPU}}
		]);

	if (show_host_hdd) {
		var id = document.getElementById('td_show_host_hdd');
		if (id)
			id.style.display = 'block';
		p.setStaples('plot_host_hdd', 'black', 'black', 1,
			[
				{{PLOT_DATA_HOST_HDD}}
			]);
	}

	p.setAxisTitle('time (ms)', 'READ       WRITE');

	xpos_width = xpos.offsetWidth;

	var xpos_caption = document.getElementById('xpos_caption');
	xpos_caption.style.padding = padding;

	xscale.style.width = xscale_width;

	xscale_fill_width = Math.round(xscale_width + Math.log(xscale_max) / Math.log(xscale_step));
	xscale_fill.style.width = xscale_fill_width;
}

var throttle = 0;

function enable_disable_left_right()
{
	if (xmin_vis == xmin && xmax_vis == xmax)
	{
		document.getElementById('btn_left').disabled = true;
		document.getElementById('btn_right').disabled = true;
	} else {
		document.getElementById('btn_left').disabled = false;
		document.getElementById('btn_right').disabled = false;
	}
}

var last_draw = 0;
function redraw()
{
	p.setRangeX(xmin_vis, xmax_vis);

	var w = xmax_vis - xmin_vis;

	// Set number of grids on X axis
	var n = 0.01;

	for (var i = n; i < 10000000; i *= 10) {
		if (i < w)
			continue;

		if (i > w * 8)
			n = i / 200;
		else if (i > w * 6)
			n = i / 100;
		else if (i > w * 4)
			n = i / 50;
		else if (i > w * 2.5)
			n = i / 40;
		else
			n = i / 20;

		p.setGrid(n, ygrid);
		break;
	}

	document.getElementById('text_xmin_vis').value = Math.round(xmin_vis * 10) / 10;
	document.getElementById('text_xmax_vis').value = Math.round(xmax_vis * 10) / 10;

	last_draw = new Date().getTime();
	p.draw();

	throttle = 0;

	return true;
}

var xpos_timer;
function scroll()
{
	var async = 0;

	if (throttle)
		return;

	throttle = 1;

	if ((new Date().getTime() - 100) < last_draw)
		async = 1;

	/*
	 * Handle IE/Safari bugs (!?) when scroll event is coming too fast
	 * Use timer.
	 */
	if (async && xpos_timer != undefined)
		clearTimeout(xpos_timer);

	/*
	 * Ok, lets process the scroll event
	 */

	var w;
	var perc;

	w = xpos_fill_width - xpos_width;
	if (w > 0)
		perc = xpos.scrollLeft / w;
	else
		perc = 0;

	w = xmax_vis - xmin_vis;
	xmin_vis = xmin + perc * (xmax - xmin - w);
	xmax_vis = xmin_vis + w;

	enable_disable_left_right();

	//console.log('scroll: ', xmin_vis, xmax_vis);
	if (async)
		xpos_timer = setTimeout(redraw, 50);
	else
		redraw();

	return true;
}

function log_scroll()
{
	p.onLogScroll();
}

function xpos_update()
{
	xpos_fill_width = xpos_width * (xmax - xmin) / (xmax_vis - xmin_vis);

	// Handle Opera Bug - scrollbar is overflowed if width >= 32768
	if (is_opera && xpos_fill_width >= 32000)
		xpos_fill_width = 32000;
	xpos_fill.style.width = xpos_fill_width;

	// Don't redraw plot
	xpos.scrollLeft = xpos_fill_width * (xmin_vis - xmin) / (xmax - xmin);

	//console.log('left: ', xpos.scrollLeft, xpos_fill_width, xmin_vis, xmax, xmin);
}

var xscale_timer;
function zoom()
{
	var async = 0;

	if (throttle)
		return false;
	throttle = 1;

	if ((new Date().getTime() - 100) < last_draw)
		async = 1;

	/*
	 * Handle IE/Safari bugs (!?) when scroll event is coming too fast
	 */
	if (xscale_timer != undefined)
		clearTimeout(xscale_timer);

	/*
	 * Ok, lets process the scroll event
	 */

	xmax_vis = xmin_vis + (xmax - xmin) / Math.pow(xscale_step, xscale.scrollLeft);

	xpos_update();
	enable_disable_left_right();

	if (async)
		xscale_timer = setTimeout(redraw, 50);
	else
		redraw();

	//opera.postError(xscale.scrollLeft);
	//console.log('zoom: ', xmin_vis, xmax_vis, xscale.scrollLeft);
	return false;
}

function xscale_update()
{
	xscale.scrollLeft = Math.log((xmax - xmin) / (xmax_vis - xmin_vis)) / Math.log(xscale_step);
	//console.log(xscale.scrollLeft, Math.log((xmax - xmin) / (xmax_vis - xmin_vis)));
}

function btn_left()
{
	var d = xmax_vis - xmin_vis;
	var x = xmin_vis - d;
	if (x < xmin)
		x = xmin;
	xmin_vis = x;
	xmax_vis = xmin_vis + d;

	throttle = 1;

	xpos_update();

	enable_disable_left_right();

	redraw();
	return false;
}

function btn_right()
{
	var d = xmax_vis - xmin_vis;
	var x = xmax_vis + d;
	if (x > xmax)
		x = xmax;
	xmax_vis = x;
	xmin_vis = x - d;

	throttle = 1;

	xpos_update();

	enable_disable_left_right();

	redraw();
	return false;
}


function btn_set_xpos()
{
	var min = parseFloat(document.getElementById('text_xmin_vis').value);
	var max = parseFloat(document.getElementById('text_xmax_vis').value);

	if (isNaN(min)) {
		alert('Left range must be an integer!');
		return;
	}

	if (isNaN(max)) {
		alert('Right range must be an integer!');
		return;
	}

	xmin_vis = min;
	xmax_vis = max;

	if (xmin_vis < xmin)
		xmin_vis = xmin;
	if (xmax_vis > xmax)
		xmax_vis = xmax;

	if (xmin_vis > xmax_vis) {
		xmin_vis = xmin;
		xmax_vis = xmax;
	}
	if (xmin_vis == xmax_vis)
		xmax_vis = xmin_vis + 0.1;

	// Throttle scroll and zoom handler
	throttle = 1;

	xpos_update();
	xscale_update();

	enable_disable_left_right();

	redraw();
	return false;
}

function load()
{
	xpos	 = document.getElementById('xpos');
	xpos_fill = document.getElementById('xpos_fill');
	xscale   	 = document.getElementById('xscale');
	xscale_fill	 = document.getElementById('xscale_fill');

	init();
	zoom();
}

</script>
</head>
<body onload="load();">
<div class=plot align=left>
	<h2>Stats</h2>

	<table>
	<tr valign=top><td align=left>
	<table class='stat'>
		<colgroup>
			<col width=80>
			<col width=80>
			<col width=100>
			<col width=80>
			<col width=100>
			<col width=100>
			<col width=100>
		</colgroup>
		</col>
		<tr>
			<th class='caption' colspan=7>Disk stat (Summary)</th>
		</tr>
		<tr>
			<th>&nbsp;</th>
			<th title="Time interval (ms) (total) ">Total (ms)</th>
			<th title="Total request complete wait time in host (total)">Wait (ms)</th>
			<th title="Number of requests (total)">Requests</th>
			<th title="Total size (MB) of requests (total)" >MBytes</th>
			<th title="Number of requests per second (total)">Req/s</th>
			<th title="I/O rate (MB/wait sec) (total)">MB/sec</th>
		</tr><tr class='clr0 disk'>
			<th>Read</th>
			<td id='st_time_3'>-</td>
			<td id='st_wait_3'>-</td>
			<td id='st_req_3'>-</td>
			<td id='st_mb_3'>-</td>
			<td id='st_req_s_3'>-</td>
			<td id='st_mb_s_3'>-</td>
		</tr>
		</tr><tr class='clr1 disk'>
			<th>Write</th>
			<td id='st_time_2'>-</td>
			<td id='st_wait_2'>-</td>
			<td id='st_req_2'>-</td>
			<td id='st_mb_2'>-</td>
			<td id='st_req_s_2'>-</td>
			<td id='st_mb_s_2'>-</td>
		</tr>
		<tr>
			<th rowspan=2>&nbsp;</th>
			<th colspan=3>Request size</th>
			<th title='sequential requests count' rowspan=2>Sequential</th>
			<th rowspan=2>Req. 4KB<br>aligned</th><th rowspan=2>Req. avg<br>size (KB)</th>
		</tr>
		<tr>
			<th> <= 4KB</th><th>4K < 1MB</th><th>64KB < 1MB</th>
		</tr><tr class='clr0 disk'>
			<th>Read</th>
			<td id='st_grp0_3'>-</td>
			<td id='st_grp1_3'>-</td>
			<td id='st_grp2_3'>-</td>
			<td id='st_seq_3'>-</td>
			<td id='st_aligned_3'>-</td>
			<td id='st_avg_3'>-</td>
		</tr>
		</tr><tr class='clr1 disk'>
			<th>Write</th>
			<td id='st_grp0_2'>-</td>
			<td id='st_grp1_2'>-</td>
			<td id='st_grp2_2'>-</td>
			<td id='st_seq_2'>-</td>
			<td id='st_aligned_2'>-</td>
			<td id='st_avg_2'>-</td>
		</tr>
		<tr>
			<th class='caption' colspan=7>Disk stat (on selected range)</th>
		</tr>
		<tr>
			<th>&nbsp;</th>
			<th title="Time interval (ms) of selected range">Range (ms)</th>
			<th title="Total request complete wait time in host (on selected range)">Wait (ms)</th>
			<th title="Number of requests (on selected range)">Requests</th>
			<th title="Total size (MB) of requests (on selected range)" >MBytes</th>
			<th title="Number of requests per second (on selected range)">Req/s</th>
			<th title="I/O rate (MB/wait sec) (on selected range)">MB/sec</th>
		</tr><tr class='clr0 disk active'>
			<th>Read</th>
			<td id='st_time_1'>-</td>
			<td id='st_wait_1'>-</td>
			<td id='st_req_1'>-</td>
			<td id='st_mb_1' title="MB read on this range / MB read before this range">-</td>
			<td id='st_req_s_1'>-</td>
			<td id='st_mb_s_1'>-</td>
		</tr>
		</tr><tr class='clr1 disk active'>
			<th>Write</th>
			<td id='st_time_0'>-</td>
			<td id='st_wait_0'>-</td>
			<td id='st_req_0'>-</td>
			<td id='st_mb_0' title="MB written on this range / MB written before this range">-</td>
			<td id='st_req_s_0'>-</td>
			<td id='st_mb_s_0'>-</td>
		</tr>
		<tr>
			<th rowspan=2>&nbsp;</th>
			<th colspan=3>Request size</th>
			<th title='sequential requests count' rowspan=2>Sequential</th>
			<th rowspan=2>Req. 4KB<br>aligned</th><th rowspan=2>Req. avg<br>size (KB)</th>
		</tr>
		<tr>
			<th> <= 4KB</th><th>4K < 1MB</th><th>>= 1MB</th>
		</tr><tr class='clr0 disk active'>
			<th>Read</th>
			<td id='st_grp0_1'>-</td>
			<td id='st_grp1_1'>-</td>
			<td id='st_grp2_1'>-</td>
			<td id='st_seq_1'>-</td>
			<td id='st_aligned_1'>-</td>
			<td id='st_avg_1'>-</td>
		</tr>
		</tr><tr class='clr1 disk active'>
			<th>Write</th>
			<td id='st_grp0_0'>-</td>
			<td id='st_grp1_0'>-</td>
			<td id='st_grp2_0'>-</td>
			<td id='st_seq_0'>-</td>
			<td id='st_aligned_0'>-</td>
			<td id='st_avg_0'>-</td>
		</tr>
	</table>

	</td><td align=left>

	<table class='stat'>
		<tr>
			<th class='caption'>Events log</th>
		</tr>
		<tr><td class='log'><pre id='log' onscroll='return log_scroll();'>{{RAW_LOG}}</pre></td></tr>
	</table>

	</td></tr></table>

	<h2>Plot</h2>
	<div id='div_plot'>
	<div id='cnv'></div>
	<div id='xpos' class='scroll' onscroll='return scroll();' style='margin-left: 60;'>
	<div id='xpos_fill' class='scroll-fill'></div></div>
	</div>

	<h2>Time interval</h2>
	<div id='xpos_caption' class='shadows' class='scroll-caption'>
		<table width='100%' class='ctrl editable'>
		<colgroup>
		<col width=20%>
		<col width=30%>
		<col width=30%>
		<col width=20%>
		</colgroup>
		<tr align=left valign=top>
		<td>
			<table class='ctrl'>
			<tr><td>Next left:</td><td>Next right:</td></tr>
			<tr><td>
				<input disabled class='button' id='btn_left' type=button onclick='return btn_left();'  value='<<'>
			</td><td>
				<input disabled class='button' id='btn_right' type=button onclick='return btn_right();' value='>>'>
			</td></tr></table>
		</td>
		<td>
			<form id='frm' onsubmit='btn_set_xpos();' action=''>
			<table class='ctrl'>
			<tr><td>Set left (ms):</td><td>Set right (ms):</td><td>Apply range:</td></tr>
			<tr><td>
				<input class='button' type=textarea id='text_xmin_vis'  value='0'>
			</td>
			<td>
				<input class='button' type=textarea id='text_xmax_vis' value='10000'>
			</td><td>
				<input class='button' type=submit onclick='return btn_set_xpos();' value='Set range'>
			</td></tr></table>
			</form>
		</td>
		<td>
			<table class='ctrl'>
			<tr><td>Zoom X scale:</td></tr>
			<tr><td>
				<div id='xscale' class='scroll' onscroll='return zoom();' width=100%><div id='xscale_fill' class='scroll-fill'></div></div>
			</td></tr></table>
		</td>
		<td>
			<table class='ctrl'>
			<tr><td align=left>
				<input type='checkbox' id='cb_plot_vm_hdd' name='cb_plot_vm_hdd' checked="yes" onchange='return redraw();'>
					Show prl_vm_app I/O activity&nbsp;
						<span><a href="javascript:void(0)" class="dsphead" onclick="dsp(this)">
						<span class='dspchar'>[+]</span></a></span>
						<div class='dspcont'>
							I/O activity in prl_vm_app<br>
							> 0 - <b>CDisk::ReadV</b> request<br>
							< 0 - <b>CDisk::WriteV</b> request
						</div>
				</input>
			</td></tr>
			<tr><td align=left id='td_show_vm_pcache' style='display: none;'>
				<input type='checkbox' id='cb_plot_vm_pcache' name='cb_plot_vm_pcache' onchange='return redraw();'>
					Show prl_vm_app PCache activity&nbsp;
						<span><a href="javascript:void(0)" class="dsphead" onclick="dsp(this)">
						<span class='dspchar'>[+]</span></a></span>
						<div class='dspcont'>
							I/O activity in prl_vm_app<br>
							> 0 - <b>CDisk::ReadV</b> request<br>
							< 0 - <b>CDisk::WriteV</b> request
						</div>
				</input>
			</td></tr>

			<tr><td align=left>
				<input type='checkbox' id='cb_plot_vm_vcpu' name='cb_plot_vm_vcpu' onchange='return redraw();'>
					Show VCPU state&nbsp;
						<span><a href="javascript:void(0)" class="dsphead" onclick="dsp(this)">
						<span class='dspchar'>[+]</span></a></span>
						<div class='dspcont'>
							VCPU plot<br>
							> 0 - VCPU thread is not idle<br>
							< 0 - VCPU thread idle
						</div>
				</input>
			</td></tr>
			<tr><td align=left id='td_show_host_hdd' style='display: none;'>
				<input type='checkbox' id='cb_plot_host_hdd' name='cb_plot_host_hdd' onchange='return redraw();'>
					Show host hw I/O activity&nbsp;
						<span><a href="javascript:void(0)" class="dsphead" onclick="dsp(this)">
						<span class='dspchar'>[+]</span></a></span>
						<div class='dspcont'>
							VCPU plot<br>
							> 0 - real hard drive <b>read</b><br>
							< 0 - real hard drive <b>write</b>
						</div>
				</input>
			</td></tr>

			</table>
		</td>
		</tr>
		</table>
	</div>

	<div id='div_info' align=right>Page is optimized for Safari @ 1440x900<br>(C) 2010 <b><font color=red>||</font> Parallels</b></div>
</div>
</body>
</html>
"""

#=============================================================================
# Stats storage
#-----------------------------------------------------------------------------

class content:
	def __init__ (self):
		self.tags = {}

#=============================================================================
# Request
#-----------------------------------------------------------------------------

class req:
	def __init__(self, addr, size, time):
		self.addr  = addr
		self.size = size
		self.begin = time
		self.end   = 0

	def complete(self, time):
		self.end = time

class queue:
	def __init__(self):
		self.reqs = deque()

	def add_req(self, addr, size, time):
		r = req(addr, size, time)
		self.reqs.append(r)
		return 0

	def complete_req(self, time):
		if len(self.reqs) == 0:
			return None
		req = self.reqs.popleft()
		req.complete(time)
		return req

	def depth(self):
		return len(self.reqs)

#=============================================================================
# Main
#-----------------------------------------------------------------------------

opt_plot_height = 350
opt_logfile = ""
opt_title = "eTrace I/O stats"
opt_size = 4
opt_show_vm_pcache = 1

if sys.platform == "darwin":
	opt_logfile = "/Library/Logs/parallels.log"
elif sys.platform == "linux2":
	opt_logfile = "/var/log/parallels.log"

def usage(opt):
	global opt_plot_height
	global opt_logfile
	global opt_title
	global opt_size

	if opt != "":
		print "Unknown option: " + opt
	print "USAGE: " + sys.argv[0] + " [ OPTIONS ] -o REPORT.HTML"
	print " where OPTIONS are:"
	print "    -f FILE  - path to parallels.log file (" + opt_logfile + " by default)"
	print "    -t TITLE - page title (" + opt_title + " by default)"
	print "    -h PIX   - plot height in pixels (" + str(opt_plot_height) + " by default)"
	print "    -r FILE  - parse HOST statistics from 'res_usage -e' log output FILE"
	print "    -s MB    - request size limit on plot (in MB) (" + str(opt_size) + " by default)"
	print "    -p       - hide pcache plot (if any)"
	print ""
	print " Note: parallels.log must include etrace log"
	sys.exit()

def time2ms(time):
	return str(int(time * 1000000) / 1000.0)

class plot_js_array:
	def __init__(self):
		self.data = ""

	def add(self, req, write):
		if req == None:
			return
		if self.data:
			self.data += ", "
		if write:
			size = -req.size
		else:
			size = req.size
		self.data += "[" + time2ms(req.begin) + \
			"," + str(size) + \
			"," + time2ms(req.end - req.begin) + \
			"," + str(req.addr) + "]"

def parse_input(fin, fres_usage):
	c = content()

	started = 0
	stopped = 0

	x_base = -1
	x_min = -1
	x_max = 0

	hdd_rd_queue = queue()
	hdd_wr_queue = queue()
	pcache_rd_queue = queue()

	req_begin = 0
	req_size = 0
	req_end = 0
	req_addr = 0

	vcpu_prev_state = -1
	hlt_begin = 0
	hlt_end = 0

	plot_data_vm_hdd = plot_js_array()
	plot_data_vm_pcache = plot_js_array()
	data_host_hdd = ""
	data_vm_vcpu = ""
	raw_log = ""

	CPU_mhz = 0
	show_vm_pcache = 0

	lines = fin.readlines()
	for l in lines:
		if "CPU freq is" in l:
			CPU_mhz = int(l.split()[3])
			continue

		if not "eTrace dump" in l:
			if stopped or not started:
				continue

		if not started:
			started = 1
			continue

		if "events found" in l:
			stopped = 1
			continue

		ar = l.split()

		if len(ar) < 4:
			raw_log += l
			continue

		if x_base == -1:
			try:
				# round to nearest sec
				x_min = float(ar[0])
				x_base = int(x_min)
			except:
				x_base = x_min = -1

		if x_base == -1:
			raw_log += l
			continue

		try:
			val = "%.3f" % (1000 * (float(ar[0]) - x_base))
			l = l.replace(ar[0], val)
		except:
			pass

		try:
			x_this = float(ar[0])
			if x_this > 0:
				x_this -= x_base
				if x_this > x_max:
					x_max = x_this
		except:
			continue

		if ar[3] == "hlt" and ar[5] == "hyp":

			raw_log += l
			if data_vm_vcpu:
				data_vm_vcpu += ", "

			hlt_begin = hlt_end
			hlt_end = float(ar[0]) - x_base
			if ar[4] == ">":
				vcpu_prev_state = 0
			elif ar[4] == "<":
				vcpu_prev_state = 1
			wait = time2ms(hlt_end - hlt_begin)
			data_vm_vcpu += "[" + time2ms(hlt_begin) + "," + \
					str(vcpu_prev_state * 1024) + ","  + wait + "]"

		elif ar[3] == "WriteEnd" or ar[3] == "ReadEnd" or ar[3] == "PCacheReadEnd":
			req_end = float(ar[0]) - x_base
			raw_log += l

			if ar[3] == "WriteEnd":
				req = hdd_wr_queue.complete_req(req_end)
				plot_data_vm_hdd.add(req, 1)
				if hdd_wr_queue.depth() == 0:
					raw_log += "</p>"
			elif ar[3] == "ReadEnd":
				req = hdd_rd_queue.complete_req(req_end)
				plot_data_vm_hdd.add(req, 0)
				if hdd_rd_queue.depth() == 0:
					raw_log += "</p>"
			elif ar[3] == "PCacheReadEnd":
				if opt_show_vm_pcache:
					req = pcache_rd_queue.complete_req(req_end)
					plot_data_vm_pcache.add(req, 0)
					if pcache_rd_queue.depth() == 0:
						raw_log += "</p>"

			if req != None:
				if req.end > x_max:
					x_max = req.end

			req_begin = 0
			req_size = 0
			x_max = req_end
		elif ar[3] == "Write" or ar[3] == "Read" or ar[3] == "PCacheRead":
			if len(ar) < 7:
				raw_log += l
				continue

			req_begin = float(ar[0]) - x_base
			req_addr = int(ar[6], 16)
			req_size = int(ar[7], 16)

			if ar[3] == "Read":
				hdd_rd_queue.add_req(req_addr, req_size, req_begin)
				raw_log += "<p class='read'>" + l
			elif ar[3] == "Write":
				hdd_wr_queue.add_req(req_addr, req_size, req_begin)
				raw_log += "<p class='write'>" + l
			elif ar[3] == "PCacheRead":
				if opt_show_vm_pcache:
					show_vm_pcache = 1
					pcache_rd_queue.add_req(req_addr, req_size, req_begin)
					raw_log += "<p class='pcache'>" + l
				else:
					raw_log += l
			
		else:
			raw_log += l

	while hdd_rd_queue.depth():
		req = hdd_rd_queue.complete_req(x_max)
		plot_data_vm_hdd.add(req, 0)

	while hdd_wr_queue.depth():
		req = hdd_wr_queue.complete_req(x_max)
		plot_data_vm_hdd.add(req, 1)

	while pcache_rd_queue.depth():
		req = pcache_rd_queue.complete_req(x_max)
		plot_data_vm_pcache.add(req, 0)

	if not hlt_begin == 0 and not vcpu_prev_state == -1:
		wait = time2ms(x_max - hlt_begin)

		if data_vm_vcpu:
			data_vm_vcpu += ", "
		data_vm_vcpu += "[" + time2ms(hlt_begin) + "," + \
				str((1 ^ vcpu_prev_state) * 1024) + ","  + wait + "]"

	#
	# Add host-side statistics
	#
	show_host_hdd = 0

	if x_base > 0 and CPU_mhz != 0 and fres_usage != None:

		show_host_hdd = 1

		lines = fres_usage.readlines()

		time	= [0, 0]
		rd_req	= [0, 0]
		wr_req	= [0, 0]
		rd_bytes= [0, 0]
		wr_bytes= [0, 0]

		for l in lines:
			ar = l.split()
			try:
				time[1] = ((int(ar[0], 16) & 0x00FFFFFFFFFFFF00) / CPU_mhz) / 1000000.0 - x_base
				rd_req[1]	= int(ar[11])
				rd_bytes[1]	= int(ar[12])
				wr_req[1]	= int(ar[16])
				wr_bytes[1]	= int(ar[17])
			except:
				continue

			d_rd_req = rd_req[1] - rd_req[0]
			d_wr_req = wr_req[1] - wr_req[0]
			d_req = d_rd_req + d_wr_req
			d_rd_bytes = rd_bytes[1] - rd_bytes[0]
			d_wr_bytes = wr_bytes[1] - wr_bytes[0]

			rd_req[0]	= rd_req[1]
			wr_req[0]	= wr_req[1]
			rd_bytes[0]	= rd_bytes[1]
			wr_bytes[0]	= wr_bytes[1]

			if time[1] < 0:
				continue

			if d_req == 0:
				time[0] = time[1]
				continue

			d_time = time[1] - time[0]

			t = time[0]
			req = 0

			dt = d_time / d_req

			while t < time[1]:
				val = 0
				if req < d_rd_req:
					val = d_rd_bytes / d_rd_req
				elif d_wr_req:
					val = -d_wr_bytes / d_wr_req

				if val:
					if data_host_hdd:
						data_host_hdd += ","
					data_host_hdd += "[" + time2ms(t) + "," + str(1024 * val) + "," + time2ms(dt) + "]"

				t += dt
				req += 1

			time[0]		= time[1]

	c.tags['PLOT_DATA_VM_HDD'] = plot_data_vm_hdd.data
	c.tags['PLOT_DATA_VM_PCACHE'] = plot_data_vm_pcache.data
	c.tags['RAW_LOG'] = raw_log

	c.tags['PLOT_DATA_VM_VCPU'] = data_vm_vcpu;
	c.tags['PLOT_DATA_HOST_HDD'] = data_host_hdd;

	c.tags['XMIN'] = time2ms(x_min - x_base);
	c.tags['XMAX'] = time2ms(x_max);
	c.tags['PLOT_HEIGHT'] = '400';
	c.tags['SHOW_HOST_HDD'] = show_host_hdd
	c.tags['SHOW_VM_PCACHE'] = show_vm_pcache * opt_show_vm_pcache

	return c

def replace_tags(content):
	global html_template
	lines = html_template.splitlines()

	ret = ""
	for l in lines:
		if "{{" in l:
			for tag in content.tags:
				pattern = "{{" + tag + "}}"
				if pattern in l:
					l = l.replace(pattern, str(content.tags[tag]))
					break
		ret += l + "\n"

	return ret

def html_report(fin, fout, fres_usage, __opt_title, __opt_plot_height, __opt_size):
	content = parse_input(fin, fres_usage)
	content.tags['TITLE'] = __opt_title
	content.tags['PLOT_HEIGHT'] = __opt_plot_height
	content.tags['REQ_SIZE'] = str(int(__opt_size) * 1024 * 1024)

	html = replace_tags(content)
	fout.write(html)

def main():
	global opt_plot_height
	global opt_logfile
	global opt_title
	global opt_size
	global opt_show_vm_pcache

	if len(sys.argv) < 3:
		usage("")

	__opt_plot_height = opt_plot_height
	__opt_title = opt_title
	__opt_logfile = opt_logfile
	__opt_size = opt_size

	try:
		opts, args = getopt.getopt(sys.argv[1:], "o:f:t:h:r:s:p")
	except getopt.GetoptError, err:
		print str(err)
		usage("")

	fout = None
	fres_usage = None

	for o, a in opts:
		if o == "-f":
			__opt_logfile = a
		elif o == "-t":
			__opt_title = a
		elif o == "-h":
			__opt_plot_height = int(a)
		elif o == "-o":
			try:
				fout = open(a, "wb")
			except IOError, err:
				print "Error: can't open file " + a
				print str(err)
				return
		elif o == "-r":
			try:
				fres_usage = open(a, "rb")
			except IOError, err:
				print "Error: can't open file " + a
				print str(err)
				return
		elif o == "-s":
			__opt_size = int(a)
		elif o == "-p":
			opt_show_vm_pcache = 0
		else:
			usage(o)

	if __opt_logfile == "":
		print "ERROR: can't file parallels.log file. Use -f option"
		usage("");

	fin = open(__opt_logfile, "rb")
	if fin == None:
		print "ERROR: can't open file " + __opt_logfile
		sys.exit()

	if fout == None:
		usage("")

	html_report(fin, fout, fres_usage, __opt_title, __opt_plot_height, __opt_size)

	fin.close()
	fout.close()

if __name__ == "__main__":
	main()

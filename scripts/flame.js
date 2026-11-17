(function () {
  var canvas = document.getElementById('flame');
  if (!canvas) return;

  var gl = canvas.getContext('webgl', { premultipliedAlpha: true, antialias: false });
  if (!gl) {
    canvas.className += ' flame-static';
    return;
  }

  var VERTEX = 'attribute vec2 p; void main() { gl_Position = vec4(p, 0.0, 1.0); }';

  var FRAGMENT = [
    'precision mediump float;',
    'uniform vec2 size;',
    'uniform float time;',
    'uniform float sway;',
    'uniform float additive;',
    'float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }',
    'float noise(vec2 p) {',
    '  vec2 i = floor(p), f = fract(p);',
    '  f = f * f * (3.0 - 2.0 * f);',
    '  return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x),',
    '             mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x), f.y);',
    '}',
    'float fbm(vec2 p) { return 0.6 * noise(p) + 0.3 * noise(p * 2.1) + 0.1 * noise(p * 4.3); }',
    'void main() {',
    // x is 0 at the wick; y is 0 at the wick and 1 at the tip of a still flame.
    '  vec2 uv = gl_FragCoord.xy / size;',
    '  float x = (uv.x - 0.5) * size.x / size.y / 0.34;',
    '  float y = (uv.y - 0.26) / 0.34;',
    '  float flicker = 0.86 + 0.14 * noise(vec2(time * 13.0, 1.7));',
    '  float t = y / (1.0 * flicker);',
    '  float tc = clamp(t, 0.0, 1.0);',
    '  float bend = sway * tc * tc * 0.5;',
    '  float wobble = (fbm(vec2(x * 2.5, y * 2.2 - time * 7.0)) - 0.5) * 0.13 * tc;',
    '  float width = 0.5 * pow(tc, 0.45) * pow(1.0 - tc, 0.75) + 0.012;',
    '  float d = abs(x - bend - wobble) / width;',
    '  float inside = step(0.0, t) * step(t, 1.0);',
    '  float body = inside * smoothstep(1.0, 0.25, d);',
    '  float core = inside * smoothstep(0.75, 0.0, d) * smoothstep(0.0, 0.25, tc);',
    '  vec3 color = mix(vec3(1.0, 0.45, 0.06), vec3(1.0, 0.82, 0.38), body);',
    '  color = mix(color, vec3(1.0, 0.97, 0.86), core);',
    '  color = mix(vec3(0.25, 0.4, 1.0), color, smoothstep(0.0, 0.22, tc));',
    '  float alpha = body * mix(0.55, 1.0, smoothstep(0.0, 0.2, tc));',
    '  vec2 g = vec2(x - bend * 0.5, (y - 0.45) * 0.8);',
    '  float glow = exp(-dot(g, g) * 1.5) * 0.42 * flicker * smoothstep(1.45, 0.5, length(g));',
    '  vec3 rgb = color * alpha + vec3(1.0, 0.55, 0.16) * glow * (1.0 - alpha);',
    '  float cover = clamp(alpha + glow * 0.6, 0.0, 1.0);',
    // The canvas is premultiplied, so colour written with less alpha adds light instead of covering.
    // On a light page only the inner flame covers; the glow and fringe add, so they never darken it.
    '  float lit = inside * smoothstep(0.9, 0.35, d);',
    '  float opacity = mix(1.0, 0.8, additive);',
    '  gl_FragColor = vec4(rgb, mix(cover, lit, additive)) * opacity;',
    '}'
  ].join('\n');

  function compile(type, source) {
    var shader = gl.createShader(type);
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    return gl.getShaderParameter(shader, gl.COMPILE_STATUS) ? shader : null;
  }

  var vs = compile(gl.VERTEX_SHADER, VERTEX);
  var fs = compile(gl.FRAGMENT_SHADER, FRAGMENT);
  var program = gl.createProgram();
  if (vs && fs) {
    gl.attachShader(program, vs);
    gl.attachShader(program, fs);
    gl.linkProgram(program);
  }
  if (!vs || !fs || !gl.getProgramParameter(program, gl.LINK_STATUS)) {
    canvas.className += ' flame-static';
    return;
  }

  gl.useProgram(program);
  gl.bindBuffer(gl.ARRAY_BUFFER, gl.createBuffer());
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 3, -1, -1, 3]), gl.STATIC_DRAW);
  var position = gl.getAttribLocation(program, 'p');
  gl.enableVertexAttribArray(position);
  gl.vertexAttribPointer(position, 2, gl.FLOAT, false, 0, 0);
  var uSize = gl.getUniformLocation(program, 'size');
  var uTime = gl.getUniformLocation(program, 'time');
  var uSway = gl.getUniformLocation(program, 'sway');
  var uAdditive = gl.getUniformLocation(program, 'additive');

  function resize() {
    var ratio = Math.min(window.devicePixelRatio || 1, 2);
    var w = Math.max(1, Math.round(canvas.clientWidth * ratio));
    var h = Math.max(1, Math.round(canvas.clientHeight * ratio));
    if (canvas.width !== w || canvas.height !== h) {
      canvas.width = w;
      canvas.height = h;
    }
    gl.viewport(0, 0, w, h);
    gl.uniform2f(uSize, w, h);
  }

  // The cursor acts as a breeze: fast movement near the flame pushes it sideways.
  var breeze = 0;
  var sway = 0;
  var lastX = null;
  window.addEventListener('mousemove', function (e) {
    if (lastX !== null) {
      var rect = canvas.getBoundingClientRect();
      var dx = e.clientX - (rect.left + rect.width / 2);
      var dy = e.clientY - (rect.top + rect.height / 2);
      var near = 120 / (120 + Math.sqrt(dx * dx + dy * dy));
      breeze += (e.clientX - lastX) * near * 0.02;
      breeze = Math.max(-1.6, Math.min(1.6, breeze));
    }
    lastX = e.clientX;
  });

  function draw(seconds) {
    resize();
    sway += (breeze - sway) * 0.12;
    breeze *= 0.93;
    gl.uniform1f(uTime, seconds);
    gl.uniform1f(uSway, sway);
    gl.uniform1f(uAdditive, document.documentElement.getAttribute('data-theme') === 'light' ? 1 : 0);
    gl.clearColor(0, 0, 0, 0);
    gl.clear(gl.COLOR_BUFFER_BIT);
    gl.drawArrays(gl.TRIANGLES, 0, 3);
  }

  var still = window.matchMedia && window.matchMedia('(prefers-reduced-motion: reduce)').matches;
  if (still) {
    draw(0);
    window.addEventListener('resize', function () { draw(0); });
    if (window.MutationObserver) {
      new MutationObserver(function () { draw(0); }).observe(document.documentElement, { attributes: true, attributeFilter: ['data-theme'] });
    }
    return;
  }

  var visible = true;
  if (window.IntersectionObserver) {
    new IntersectionObserver(function (entries) {
      visible = entries[0].isIntersecting;
    }).observe(canvas);
  }

  function frame(ms) {
    if (visible) draw(ms / 1000);
    requestAnimationFrame(frame);
  }
  requestAnimationFrame(frame);
})();

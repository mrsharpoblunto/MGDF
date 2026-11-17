(function () {
  var root = document.documentElement;

  // Theme: an explicit choice is stored; otherwise the page follows the system setting.
  var themeToggle = document.querySelector('[data-theme-toggle]');
  if (themeToggle) {
    themeToggle.addEventListener('click', function () {
      var theme = root.getAttribute('data-theme') === 'light' ? 'dark' : 'light';
      root.setAttribute('data-theme', theme);
      try { localStorage.setItem('theme', theme); } catch (e) {}
    });
  }
  if (window.matchMedia) {
    var light = window.matchMedia('(prefers-color-scheme: light)');
    var follow = function () {
      var stored;
      try { stored = localStorage.getItem('theme'); } catch (e) {}
      if (stored !== 'light' && stored !== 'dark') {
        root.setAttribute('data-theme', light.matches ? 'light' : 'dark');
      }
    };
    if (light.addEventListener) light.addEventListener('change', follow);
  }

  function setExpanded(button, target, open) {
    button.setAttribute('aria-expanded', open ? 'true' : 'false');
    target.classList.toggle('hidden', !open);
  }

  var navToggle = document.querySelector('[data-nav-toggle]');
  if (navToggle) {
    var siteNav = document.getElementById(navToggle.getAttribute('aria-controls'));
    navToggle.addEventListener('click', function () {
      setExpanded(navToggle, siteNav, navToggle.getAttribute('aria-expanded') !== 'true');
    });
  }

  var dropdowns = [].slice.call(document.querySelectorAll('[data-dropdown]'));
  dropdowns.forEach(function (button) {
    var menu = document.getElementById(button.getAttribute('aria-controls'));
    button.addEventListener('click', function (e) {
      e.stopPropagation();
      setExpanded(button, menu, button.getAttribute('aria-expanded') !== 'true');
    });
  });
  function closeDropdowns() {
    dropdowns.forEach(function (button) {
      setExpanded(button, document.getElementById(button.getAttribute('aria-controls')), false);
    });
  }
  document.addEventListener('click', closeDropdowns);
  document.addEventListener('keydown', function (e) {
    if (e.key === 'Escape') closeDropdowns();
  });

  // Scrollspy: marks the side nav entry (and its parent entries) for the section at the top of the viewport.
  var nav = document.querySelector('.side-nav');
  if (!nav) return;

  var OFFSET = 80;
  var entries = [].slice.call(nav.querySelectorAll('a[href^="#"]')).map(function (link) {
    return { item: link.parentNode, target: document.getElementById(decodeURIComponent(link.hash.slice(1))) };
  }).filter(function (entry) { return entry.target; });

  var tops = [];
  var measuredHeight = -1;
  function measure() {
    measuredHeight = document.documentElement.scrollHeight;
    tops = entries.map(function (entry) {
      return entry.target.getBoundingClientRect().top + window.scrollY;
    });
  }

  var active = null;
  function activate(entry) {
    if (entry === active) return;
    active = entry;
    [].forEach.call(nav.querySelectorAll('li.active'), function (li) { li.classList.remove('active'); });
    if (!entry) return;
    for (var li = entry.item; li && li !== nav; li = li.parentNode) {
      if (li.tagName === 'LI') li.classList.add('active');
    }
    if (nav.classList.contains('api-nav')) keepVisible(entry.item);
  }

  // Keeps the current entry visible in the API sidebar while the page scrolls.
  function keepVisible(item) {
    if (item.querySelector('ul') || nav.matches(':hover')) return;
    var top = item.firstElementChild.getBoundingClientRect().top - nav.getBoundingClientRect().top + nav.scrollTop;
    if (top < nav.scrollTop + 60 || top > nav.scrollTop + nav.clientHeight - 60) {
      nav.scrollTop = top - nav.clientHeight / 2;
    }
  }

  function spy() {
    if (document.documentElement.scrollHeight !== measuredHeight) measure();
    var y = window.scrollY + OFFSET;
    var bottom = window.scrollY + window.innerHeight >= measuredHeight - 1;
    var best = null;
    var bestTop = -Infinity;
    for (var i = 0; i < entries.length; i++) {
      if (tops[i] <= y && tops[i] >= bestTop) {
        best = entries[i];
        bestTop = tops[i];
      }
    }
    if (bottom && entries.length) best = entries[entries.length - 1];
    activate(best || entries[0]);
  }

  var pending = false;
  function schedule() {
    if (pending) return;
    pending = true;
    requestAnimationFrame(function () {
      pending = false;
      spy();
    });
  }
  window.addEventListener('scroll', schedule, { passive: true });
  window.addEventListener('resize', function () { measuredHeight = -1; schedule(); });
  window.addEventListener('load', function () { measuredHeight = -1; schedule(); });
  spy();

  var filter = document.getElementById('api-filter');
  if (filter) {
    filter.addEventListener('input', function () {
      var term = filter.value.toLowerCase();
      [].forEach.call(nav.querySelectorAll('ul ul li'), function (li) {
        li.classList.toggle('hidden', li.textContent.toLowerCase().indexOf(term) === -1);
      });
    });
  }
})();

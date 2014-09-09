(function ($) {
	$.fn.biscroll = function (options) {
		var defaults = {
			affixClass: 'affix'
		};
		var options = $.extend({}, defaults, options);

		var nav = this;
		var main = $(options.main);
		var w = $(window);

		var windowHeight = w.height();
		var footerHeight = $(options.footer).outerHeight(true);
		var mainTop = main.offset().top;
		var navTop = nav.offset().top;

		function onresize()
		{
			windowHeight = w.height();
			onscroll();
		}

		function onscroll()
		{
			if (nav.height() + navTop > windowHeight) {
				var mainHeight = main.height() - windowHeight - mainTop;
				var scrollPosition = w.scrollTop();

				var percentage = Math.min(1.0, Math.max(0, (scrollPosition / mainHeight)));
				var navScroll = nav.height() - windowHeight + footerHeight + navTop;

				nav.css('top', (navTop - (navScroll * percentage)) + 'px');
			} else {
				nav.css('top', '');
			}
		}

		w.resize(onresize);
		w.scroll(onscroll);

		nav.addClass(options.affixClass);
		onscroll();
		return this;
	};
}(jQuery));
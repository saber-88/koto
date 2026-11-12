/* Background depth: a fixed grid layer on the landing page that lights up around the cursor. Pure decoration (aria-hidden). */
(function () {
  function init() {
    // Only mount on the home landing page (identified by .koto-hero and absence of docs sidebar)
    if (!document.querySelector('.koto-hero') || document.documentElement.hasAttribute('data-has-sidebar')) {
      return;
    }

    var fx = document.createElement('div');
    fx.className = 'koto-fx';
    fx.setAttribute('aria-hidden', 'true');
    document.body.prepend(fx);
    if (matchMedia('(prefers-reduced-motion: reduce)').matches) return;

    var mx = 0, my = 0, moving = false, scrolling = false;
    addEventListener('pointermove', function (e) {
      mx = e.clientX; my = e.clientY;
      if (moving) return;
      moving = true;
      requestAnimationFrame(function () {
        moving = false;
        fx.style.setProperty('--mx', mx + 'px');
        fx.style.setProperty('--my', my + 'px');
      });
    }, { passive: true });

    addEventListener('scroll', function () {
      if (scrolling) return;
      scrolling = true;
      requestAnimationFrame(function () {
        scrolling = false;
        fx.style.setProperty('--sy', scrollY + 'px');
      });
    }, { passive: true });
  }
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', init);
  else init();
})();

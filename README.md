# matchstickframework.org

Jekyll site for the Matchstick docs, served by GitHub Pages.

The stylesheet is compiled with Tailwind. GitHub Pages does not run it, so commit the compiled `assets/css/site.css` whenever the Tailwind source or the classes in any page change:

```
npm install
npm run build     # or: npm run watch
```

The source is `_tailwind/site.css`. Theme colours are CSS variables on `:root` (dark) and `:root[data-theme="light"]`.

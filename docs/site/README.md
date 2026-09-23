# Website

GitHub Pages serves `docs/`. Its entry page is `docs/index.html`; all supporting
website files live here, with paths relative to that entry page.

- `styles.css`: shared site styles.
- `assets/detector/`: the seven detector renders, ordered outside to inside on the homepage.

Place future exported marimo pages under `site/notebooks/` and demos under
`site/demos/`, each with its own assets if needed. Add links from the homepage
when those pages exist. No build step or JavaScript is required for the gallery.
The older images in `docs/figures/` support the technical documentation and are
not used by the site.

Preview from the repository root:

```sh
python -m http.server 8000 --directory docs
```

Open `http://localhost:8000/`. Each gallery image links to its full-size PNG.

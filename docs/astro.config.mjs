import { defineConfig, passthroughImageService } from 'astro/config';
import starlight from '@astrojs/starlight';
import sitemap from '@astrojs/sitemap';
import mermaid from 'astro-mermaid';
import { config } from './src/config.ts';
import { remarkKoto } from './src/plugins/remark-koto.mjs';

const ogImage = `${config.site}${config.basePrefix}/social-card.png`;

export default defineConfig({
  site: config.site,
  base: config.base,
  output: 'static',
  markdown: {
    remarkPlugins: [
      [remarkKoto, { basePrefix: config.basePrefix, repoUrl: config.repoUrl }],
    ],
  },
  integrations: [
    // must come before starlight so ```mermaid fences are turned into diagrams
    mermaid({ autoTheme: true, mermaidConfig: { fontFamily: 'JetBrains Mono NL, ui-monospace, monospace', flowchart: { htmlLabels: true, curve: 'linear' } } }),
    starlight({
      title: config.name,
      description: config.description,
      logo: {
        src: './src/assets/logo.svg',
        alt: 'libkoto logo',
        replacesTitle: false,
      },
      favicon: '/favicon.svg',
      social: [
        { icon: 'github', label: `${config.name} on GitHub`, href: config.repoUrl },
      ],
      head: [
        { tag: 'script', attrs: { src: `${config.basePrefix}/koto-fx.js`, defer: true } },
        { tag: 'meta', attrs: { property: 'og:image', content: ogImage } },
        { tag: 'meta', attrs: { property: 'og:image:width', content: '1200' } },
        { tag: 'meta', attrs: { property: 'og:image:height', content: '630' } },
        {
          tag: 'meta',
          attrs: { property: 'og:image:alt', content: `${config.name} — ${config.tagline}` },
        },
        { tag: 'meta', attrs: { name: 'twitter:image', content: ogImage } },
        { tag: 'meta', attrs: { name: 'theme-color', content: '#0f1217' } },
        { tag: 'meta', attrs: { name: 'color-scheme', content: 'light dark' } },
        {
          tag: 'link',
          attrs: { rel: 'apple-touch-icon', href: `${config.basePrefix}/apple-touch-icon.png` },
        },
      ],
      sidebar: [
        {
          label: 'Getting started',
          items: [
            { slug: 'getting-started/requirements', label: 'Requirements' },
            { slug: 'getting-started/installation', label: 'Installation' },
            { slug: 'getting-started/first-app', label: 'Your first app' },
            { slug: 'getting-started/building-running', label: 'Building & running' },
          ],
        },
        {
          label: 'Concepts',
          items: [
            { slug: 'concepts/immediate-mode', label: 'Immediate-mode rendering' },
            { slug: 'concepts/frames-diff-renderer', label: 'Frames & the diff renderer' },
            { slug: 'concepts/ids-hit-buffer', label: 'Ids & the hit buffer' },
            { slug: 'concepts/events-input', label: 'Events & input' },
            { slug: 'concepts/focus-management', label: 'Focus management' },
            { slug: 'concepts/layout', label: 'Layout' },
          ],
        },
        {
          label: 'Architecture',
          // any .md dropped in src/content/docs/architecture/ appears here, ordered by its `sidebar.order`
          items: [{ autogenerate: { directory: 'architecture' } }],
        },
        {
          label: 'Guides',
          items: [
            { slug: 'guides/layout-recipes', label: 'Layout recipes' },
            { slug: 'guides/keyboard', label: 'Handling keyboard input' },
            { slug: 'guides/mouse', label: 'Mouse & click-to-focus' },
            { slug: 'guides/modal-dialogs', label: 'Focus & modal dialogs' },
            { slug: 'guides/colors', label: 'Colors' },
            { slug: 'guides/unicode', label: 'Unicode & wide characters' },
            { slug: 'guides/headless-testing', label: 'Headless testing' },
          ],
        },
        {
          label: 'Deep dives',
          items: [{ autogenerate: { directory: 'deep-dives' } }],   // same for src/content/docs/deep-dives/
        },
        {
          label: 'Widgets reference',
          collapsed: true,
          items: [
            { slug: 'widgets/box', label: 'Box' },
            { slug: 'widgets/text', label: 'Text' },
            { slug: 'widgets/rule', label: 'Rule' },
            { slug: 'widgets/fill', label: 'Fill & Clear' },
            { slug: 'widgets/gauge', label: 'Gauge' },
            { slug: 'widgets/sparkline', label: 'Sparkline' },
            { slug: 'widgets/vbox', label: 'Vbox' },
            { slug: 'widgets/hbox', label: 'Hbox' },
            { slug: 'widgets/list', label: 'List' },
            { slug: 'widgets/text-input', label: 'TextInput' },
            { slug: 'widgets/text-area', label: 'TextArea' },
            { slug: 'widgets/tabs', label: 'Tabs' },
            { slug: 'widgets/button', label: 'Button' },
          ],
        },
        {
          label: 'API reference',
          collapsed: true,
          items: [
            { slug: 'api/app', label: 'App' },
            { slug: 'api/terminal', label: 'Terminal' },
            { slug: 'api/frame', label: 'Frame' },
            { slug: 'api/cell-color', label: 'Cell & Color' },
            { slug: 'api/events', label: 'Event, KeyEvent & MouseEvent' },
            { slug: 'api/focus-manager', label: 'FocusManager' },
            { slug: 'api/layout', label: 'Layout' },
            { slug: 'api/id', label: 'Id' },
            { slug: 'api/utf8', label: 'UTF-8 helpers' },
            { slug: 'api/io', label: 'I/O & Widget Base' },
          ],
        },
        { slug: 'limitations-faq', label: 'Limitations & FAQ' },
        { slug: 'changelog', label: 'Changelog' },
      ],
      editLink: { baseUrl: config.editUrl },
      customCss: [
        '@fontsource-variable/inter-tight/wght.css',
        '@fontsource-variable/source-serif-4/wght.css',
        '@fontsource-variable/source-serif-4/wght-italic.css',
        './src/styles/theme.css',
      ],
      components: {
        PageTitle: './src/components/PageTitle.astro',
      },
      expressiveCode: {
        themes: ['one-dark-pro', 'github-light'],
        styleOverrides: {
          borderRadius: '6px',
          borderColor: { dark: '#222938', light: '#e2e8f0' },
          frames: {
            frameBoxShadowCssValue: 'none',
            editorBackground: { dark: '#151922', light: '#ffffff' },
            terminalBackground: { dark: '#12161f', light: '#f8fafc' },
            editorTabBarBackground: { dark: '#1a202c', light: '#f1f5f9' },
            terminalTitlebarBackground: { dark: '#161c26', light: '#f1f5f9' },
          },
          codeFontFamily: 'var(--sl-font-mono)',
        },
      },
    }),
    sitemap(),
  ],
  image: {
    service: passthroughImageService(),
  },
});

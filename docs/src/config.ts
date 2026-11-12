/**
 * Single source of truth for site-wide values.
 *
 * Every value can be overridden with an environment variable, so CI/hosting
 * can inject the right URLs without editing source files:
 *
 *   SITE_URL   canonical origin, e.g. https://you.github.io (Netlify's URL is used as a fallback)
 *   BASE_PATH  sub-path the site is served from: "/koto" on GitHub project pages, "/" at a domain root
 *   REPO_URL   GitHub repository of the library
 *   EDIT_URL   base URL for "Edit this page" links (must end at src/content/docs/)
 *
 * The defaults below are what you get on a plain local `npm run dev`.
 * Replace OWNER with your GitHub username/organisation.
 */
type Env = Record<string, string | undefined>;
const env: Env =
  (globalThis as unknown as { process?: { env?: Env } }).process?.env ?? {};

function normalizeBase(raw: string): string {
  let base = raw.trim();
  if (base === '' || base === '/') return '/';
  if (!base.startsWith('/')) base = `/${base}`;
  return base.replace(/\/+$/, '');
}

const repoUrl = (env.REPO_URL || 'https://github.com/saber-88/koto').replace(/\/+$/, '');
const base = normalizeBase(env.BASE_PATH ?? '/');

export interface KotoConfig {
  name: string;
  shortName: string;
  tagline: string;
  description: string;
  repoUrl: string;
  editUrl: string;
  version: string;
  site: string;
  /** "/" or "/something" (never a trailing slash except for "/") */
  base: string;
  /** base without the trailing slash, safe to concatenate: "" or "/koto" */
  basePrefix: string;
}

export const config: KotoConfig = {
  name: 'libkoto',
  shortName: 'koto',
  tagline: 'a terminal UI library for modern C++',
  description:
    'libkoto (koto) is a dependency-free, immediate-mode terminal UI library for modern C++ with 24-bit color, keyboard and mouse input, and headless testing.',
  repoUrl,
  editUrl: (env.EDIT_URL || `${repoUrl}/edit/main/docs/src/content/docs/`).replace(/\/*$/, '/'),
  version: '1.0.0',
  site: (env.SITE_URL || env.URL || 'https://koto.karmveervaghela.workers.dev').replace(/\/+$/, ''),
  base,
  basePrefix: base === '/' ? '' : base,
};

export const REPO_URL = config.repoUrl;
export const VERSION = config.version;

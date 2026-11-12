/**
 * Remark plugin used for all Markdown/MDX pages.
 *
 * 1. Prefixes root-relative links ("/guides/colors/") with the site's base path,
 *    so docs can be written without caring where the site is hosted
 *    (GitHub project pages serve from "/koto", Netlify from "/").
 * 2. Replaces the placeholder repository URL used in examples with the real one
 *    from src/config.ts, so there is exactly one place to set it.
 */
const PLACEHOLDER_REPO = 'https://github.com/OWNER/koto';

export function remarkKoto({ basePrefix = '', repoUrl = PLACEHOLDER_REPO } = {}) {
  const swapRepo = (s) => (typeof s === 'string' ? s.split(PLACEHOLDER_REPO).join(repoUrl) : s);

  const fixUrl = (url) => {
    if (typeof url !== 'string') return url;
    url = swapRepo(url);
    if (!basePrefix) return url;
    const isRootRelative = url.startsWith('/') && !url.startsWith('//');
    const alreadyPrefixed = url === basePrefix || url.startsWith(`${basePrefix}/`);
    return isRootRelative && !alreadyPrefixed ? basePrefix + url : url;
  };

  const walk = (node) => {
    if (!node || typeof node !== 'object') return;
    if (Array.isArray(node.children)) {
      node.children = node.children.filter((child) => {
        if (child.type === 'paragraph' && child.children?.[0]?.type === 'text') {
          const val = child.children[0].value.trim();
          if (val.startsWith('import ') && val.includes('starlight/components')) {
            return false;
          }
        }
        return true;
      });
      node.children.forEach(walk);
    }
    switch (node.type) {
      case 'link':
      case 'definition':
        node.url = fixUrl(node.url);
        break;
      case 'mdxJsxFlowElement':
      case 'mdxJsxTextElement':
        for (const attr of node.attributes ?? []) {
          if (attr.type === 'mdxJsxAttribute' && attr.name === 'href' && typeof attr.value === 'string') {
            attr.value = fixUrl(attr.value);
          }
        }
        break;
      case 'html':
        node.value = swapRepo(node.value).replace(/(href=")(\/[^"]*)(")/g, (_, a, u, c) => a + fixUrl(u) + c);
        break;
      case 'code':
      case 'inlineCode':
      case 'text':
        node.value = swapRepo(node.value);
        break;
    }
  };

  return (tree) => walk(tree);
}

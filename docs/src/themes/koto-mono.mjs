// Monochromatic syntax themes: grayscale plus metallic silver accent. Keywords, preprocessor and language constants
// carry the bright platinum silver accent; everything else is brightness, weight and slant.
const rule = (scope, foreground, fontStyle) => ({
  scope, settings: fontStyle ? { foreground, fontStyle } : { foreground },
});

function make({ name, type, ui, c }) {
  return {
    name, type,
    colors: {
      'editor.background': ui.bg,
      'editor.foreground': c.base,
      'editorLineNumber.foreground': ui.dim,
      'editorGroupHeader.tabsBackground': ui.chrome,
      'editorGroupHeader.tabsBorder': ui.line,
      'tab.activeBackground': ui.bg,
      'tab.inactiveBackground': ui.chrome,
      'tab.activeForeground': c.base,
      'tab.inactiveForeground': ui.dim,
      'tab.border': ui.line,
      'tab.activeBorder': ui.bg,
      'tab.activeBorderTop': ui.accent,
      'titleBar.activeBackground': ui.chrome,
      'titleBar.activeForeground': c.base,
      'titleBar.border': ui.line,
      'terminal.background': ui.term,
      'terminal.foreground': c.base,
      'editorGroup.border': ui.line,
      'scrollbarSlider.background': ui.line,
      'scrollbarSlider.hoverBackground': ui.dim,
      'focusBorder': ui.accent,
      'editor.selectionBackground': ui.line,
      'editor.wordHighlightBackground': ui.line,
      'editor.findMatchHighlightBackground': ui.line,
      'editorWidget.background': ui.chrome,
      'editorWidget.border': ui.line,
      'button.background': ui.line,
      'button.foreground': c.base,
    },
    tokenColors: [
      rule(['comment', 'punctuation.definition.comment'], c.comment, 'italic'),
      rule(['keyword', 'keyword.control', 'storage', 'storage.modifier', 'keyword.other'], c.accent, 'bold'), rule(['storage.type'], c.accent),
      rule(['keyword.operator', 'punctuation', 'meta.brace', 'punctuation.separator', 'punctuation.terminator'], c.punct),
      rule(['entity.name.type', 'entity.name.class', 'entity.name.struct', 'entity.name.enum', 'support.type', 'support.class'], c.strong),
      rule(['entity.name.namespace', 'entity.name.scope-resolution'], c.soft),
      rule(['entity.name.function', 'support.function', 'meta.function-call', 'variable.function'], c.fn),
      rule(['string', 'string.quoted', 'punctuation.definition.string'], c.string),
      rule(['constant.numeric', 'constant.character', 'constant.other'], c.strong),
      rule(['constant.language', 'support.constant'], c.accent, 'italic'),
      rule(['meta.preprocessor', 'keyword.control.directive', 'punctuation.definition.directive'], c.accent),
      rule(['variable', 'variable.other', 'variable.parameter', 'meta.definition.variable'], c.base),
      rule(['markup.heading', 'entity.name.section'], c.strong, 'bold'),
      rule(['markup.inserted'], c.strong), rule(['markup.deleted'], c.comment),
    ],
  };
}

export const kotoDark = make({
  name: 'koto-dark', type: 'dark',
  ui: { bg: '#1c1c1c', term: '#000000', chrome: '#252525', line: '#313131', dim: '#7a7a7a', accent: '#ffffff' },
  c: { base: '#c5c5c5', strong: '#ffffff', fn: '#e5e5e5', string: '#9a9a9a', punct: '#7a7a7a', soft: '#b0b0b0', comment: '#686868', accent: '#ffffff' },
});

export const kotoLight = make({
  name: 'koto-light', type: 'light',
  ui: { bg: '#f4f4f2', term: '#ffffff', chrome: '#e9e9e6', line: '#d4d4d0', dim: '#8a8a86', accent: '#0f172a' },
  c: { base: '#2b2b2b', strong: '#000000', fn: '#161616', string: '#666666', punct: '#767676', soft: '#4a4a4a', comment: '#8c8c88', accent: '#0f172a' },
});

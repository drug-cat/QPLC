# QPLC — VS Code Extension (placeholder)

This folder contains a scaffold for the QPLC VS Code extension.

## Contents

- `package.json` — extension manifest
- `language-configuration.json` — indentation + auto-closing
- `syntaxes/qplc.tmGrammar.json` — TextMate grammar (syntax highlighting)
- `client/` (soon) — LSP client that connects to `qplc --lsp`

## Local install

```bash
cd vscode
npm install -g @vscode/vsce
vsce package
code --install-extension qplc-0.2.0.vsix
```

## Enabling LSP

For full LSP support, a client must be written in `client/extension.ts`
that connects to `qplc --lsp`. Currently only syntax highlighting is available.

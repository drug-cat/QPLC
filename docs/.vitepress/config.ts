import { defineConfig } from 'vitepress'

export default defineConfig({
  title: 'QPLC',
  description: 'Industrial ladder logic DSL that compiles to Ladder Logic and SCL, with a simulator, Modbus TCP, and a time-travel debugger.',
  lang: 'en-US',
  base: '/QPLC/',
  themeConfig: {
    nav: [
      { text: 'Guide', link: '/guide/installation' },
      { text: 'Language', link: '/guide/language' },
      { text: 'API', link: '/api/dotnet' },
      { text: 'GitHub', link: 'https://github.com/YOUR_USERNAME/QPLC' },
    ],
    sidebar: [
      {
        text: 'Guide',
        items: [
          { text: 'Installation', link: '/guide/installation' },
          { text: 'Language Reference', link: '/guide/language' },
          { text: 'Examples', link: '/guide/examples' },
          { text: 'Modbus TCP', link: '/guide/modbus' },
        ],
      },
      {
        text: 'API',
        items: [
          { text: '.NET (QPLC.Core)', link: '/api/dotnet' },
        ],
      },
    ],
    socialLinks: [{ icon: 'github', link: 'https://github.com/YOUR_USERNAME/QPLC' }],
    footer: {
      message: 'Released under the Apache-2.0 License',
      copyright: 'Copyright © 2026 QPLC Contributors',
    },
  },
})

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
        text: 'کتاب',
        items: [
          { text: 'مقدمه', link: '/book/' },
          { text: 'فصل ۱ — شروع', link: '/book/ch01-getting-started' },
          { text: 'فصل ۲ — مفاهیم پایه', link: '/book/ch02-basic-concepts' },
          { text: 'فصل ۳ — کنترل جریان', link: '/book/ch03-control-flow' },
          { text: 'فصل ۴ — توابع', link: '/book/ch04-functions' },
          { text: 'فصل ۵ — تایمر/شمارنده', link: '/book/ch05-timers-counters' },
          { text: 'فصل ۶ — struct/enum', link: '/book/ch06-struct-enum' },
          { text: 'فصل ۷ — رشته‌ها', link: '/book/ch07-strings' },
          { text: 'فصل ۸ — ماژول‌ها', link: '/book/ch08-modules' },
          { text: 'فصل ۹ — مدیریت خطا', link: '/book/ch09-errors' },
          { text: 'فصل ۱۰ — پروژه واقعی', link: '/book/ch10-real-project' },
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

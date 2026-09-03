import { defineConfig } from "vitepress";
import { withMermaid } from "vitepress-plugin-mermaid";

const getCurrentYear = () => {
  const date = new Date();
  return date.getFullYear();
};

// https://vitepress.dev/reference/site-config
export default withMermaid(
  defineConfig({
    lang: "zh-CN",
    title: "PLand",
    description: "PLand - 基于 C++ 开发的领地插件",
    base: "/PLand/",

    // 移除主页，将站点根路径重写到安装指南
    rewrites: {
      "user_guide/QuickStart.md": "index.md",
    },

    themeConfig: {
      // https://vitepress.dev/reference/default-theme-config
      nav: [
        {
          text: "更新日志",
          link: "https://github.com/IceBlcokMC/PLand/blob/main/CHANGELOG.md",
        },
        { text: "GitHub", link: "https://github.com/IceBlcokMC/PLand" },
      ],

      sidebar: [
        {
          text: "用户指南",
          items: [
            { text: "快速上手", link: "/user_guide/QuickStart.md" },
            { text: "安装指南", link: "/user_guide/Install.md" },
            { text: "指令列表", link: "/user_guide/CommandList.md" },
            { text: "领地配置文件", link: "/user_guide/Config.md" },
            {
              text: "拦截器配置文件",
              link: "/user_guide/InterceptorConfig.md",
            },
            { text: "术语表", link: "/user_guide/Glossary.md" },
            { text: "疑难解答", link: "/user_guide/FAQ.md" },
            { text: "多语言", link: "/user_guide/I18n.md" },
          ],
        },
        {
          text: "开发者指南",
          items: [
            {
              text: "C++ 原生开发",
              collapsed: false,
              items: [
                { text: "快速开始", link: "/developer_guide/QuickStart.md" },
                { text: "环境配置", link: "/developer_guide/RequireSDK.md" },
                { text: "接口指南", link: "/developer_guide/LDAPI.md" },
                { text: "事件定义", link: "/developer_guide/Event.md" },
              ],
            },
            {
              text: "脚本开发(PLand-LRCA)",
              collapsed: false,
              items: [
                {
                  text: "概览",
                  link: "/developer_guide/script/lrca/index.md",
                },
                {
                  text: "接口定义",
                  collapsed: true,
                  items: [
                    {
                      text: "类型定义",
                      link: "/developer_guide/script/lrca/api/types.md",
                    },
                    {
                      text: "领地对象(Land)",
                      link: "/developer_guide/script/lrca/api/land.md",
                    },
                    {
                      text: "领地注册表(LandRegistry)",
                      link: "/developer_guide/script/lrca/api/land_registry.md",
                    },
                    {
                      text: "领地范围(LandAABB)",
                      link: "/developer_guide/script/lrca/api/land_aabb.md",
                    },
                    {
                      text: "租赁服务(LeasingService)",
                      link: "/developer_guide/script/lrca/api/leasing_service.md",
                    },
                    {
                      text: "脚本事件系统(Event)",
                      link: "/developer_guide/script/lrca/api/event.md",
                    },
                    {
                      text: "其它接口",
                      link: "/developer_guide/script/lrca/api/other.md",
                    },
                  ],
                },
              ],
            },
          ],
        },
        {
          text: "设计文档",
          items: [{ text: "租赁模式", link: "/design/feat/LeasingModel.md" }],
        },
        {
          text: "其他",
          items: [
            {
              text: "更新日志",
              link: "https://github.com/IceBlcokMC/PLand/blob/main/CHANGELOG.md",
            },
            { text: "QQ 群", link: "https://qm.qq.com/q/v2faa5B2xk" },
            {
              text: "Bug 反馈(新功能请求)",
              link: "https://github.com/IceBlcokMC/PLand/issues",
            },
          ],
        },
      ],

      // 本地搜索
      search: {
        provider: "local",
        options: {
          translations: {
            button: {
              buttonText: "搜索",
              buttonAriaLabel: "搜索文档",
            },
            modal: {
              noResultsText: "找不到相关结果",
              resetButtonTitle: "清除查询条件",
              footer: {
                navigateText: "切换",
                selectText: "选择",
                closeText: "关闭",
              },
            },
          },
        },
      },

      docFooter: {
        prev: "上一页",
        next: "下一页",
      },

      lastUpdated: {
        text: "最后更新于",
        formatOptions: {
          dateStyle: "full",
          timeStyle: "medium",
        },
      },

      outline: {
        label: "本页目录",
        level: [2, 3],
      },

      returnToTopLabel: "回到顶部",
      sidebarMenuLabel: "菜单",
      darkModeSwitchLabel: "主题",
      lightModeSwitchTitle: "切换到浅色模式",
      darkModeSwitchTitle: "切换到深色模式",

      notFound: {
        title: "页面未找到",
        quote: "这个页面不见了 (⋟﹏⋞) 可能页面被删除或者迁移到其它位置了",
        linkLabel: "返回首页",
        linkText: "回到首页",
        code: "404",
      },

      // 编辑此页链接
      editLink: {
        pattern: "https://github.com/IceBlcokMC/PLand/edit/main/docs/:path",
        text: "编辑此页",
      },

      footer: {
        message: "AGPL-3.0 Licensed",
        copyright: `Copyright © 2024-${getCurrentYear()} PLand Contributors`,
      },
    },
  }),
);

<script setup lang="ts">
import DefaultTheme from 'vitepress/theme';
import { nextTick, onBeforeUnmount, onMounted, onUpdated } from 'vue';

const { Layout } = DefaultTheme;

/* --------------------------------------------------------------------------
 * 自定义容器：无自定义标题时，把默认的英文类型名替换为中文
 * ------------------------------------------------------------------------ */
const BLOCK_LABELS: Record<string, string> = {
    tip: '提示',
    info: '信息',
    warning: '注意',
    danger: '危险',
    note: '备注',
    important: '重要',
    caution: '小心',
    details: '展开详情',
};

const DEFAULT_TITLES = new Set([
    'TIP',
    'INFO',
    'WARNING',
    'DANGER',
    'NOTE',
    'IMPORTANT',
    'CAUTION',
    'Details',
]);

function enhanceCustomBlocks() {
    document.querySelectorAll<HTMLElement>('.vp-doc .custom-block').forEach((block) => {
        const type = Object.keys(BLOCK_LABELS).find((t) => block.classList.contains(t));
        if (!type) return;

        if (block.tagName === 'DETAILS') {
            const summary = block.querySelector('summary');
            if (summary && DEFAULT_TITLES.has(summary.textContent?.trim() ?? '')) {
                summary.textContent = BLOCK_LABELS[type];
            }
            return;
        }

        const title = block.querySelector<HTMLElement>('.custom-block-title');
        if (title && DEFAULT_TITLES.has(title.textContent?.trim() ?? '')) {
            title.textContent = BLOCK_LABELS[type];
        }
    });
}

/* --------------------------------------------------------------------------
 * 右下角一键回顶悬浮按钮
 * ------------------------------------------------------------------------ */
const SHOW_AFTER = 400;

let backTopBtn: HTMLButtonElement | null = null;

function updateBackTop() {
    if (backTopBtn) {
        backTopBtn.classList.toggle('visible', window.scrollY > SHOW_AFTER);
    }
}

function scrollToTop() {
    window.scrollTo({ top: 0, behavior: 'smooth' });
}

function setupBackToTop() {
    const btn = document.createElement('button');
    btn.className = 'vp-back-to-top';
    btn.type = 'button';
    btn.setAttribute('aria-label', '回到顶部');
    btn.title = '回到顶部';
    btn.innerHTML =
        '<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M18 15l-6-6-6 6"/></svg>';
    btn.addEventListener('click', scrollToTop);
    document.body.appendChild(btn);
    backTopBtn = btn;
    updateBackTop();
    window.addEventListener('scroll', updateBackTop, { passive: true });
}

function teardownBackToTop() {
    window.removeEventListener('scroll', updateBackTop);
    backTopBtn?.remove();
    backTopBtn = null;
}

onMounted(() => {
    setupBackToTop();
    nextTick(enhanceCustomBlocks);
});

onUpdated(() => {
    nextTick(enhanceCustomBlocks);
});

onBeforeUnmount(teardownBackToTop);
</script>

<template>
    <Layout />
</template>

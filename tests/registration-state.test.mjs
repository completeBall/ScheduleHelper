import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';

const source = fs.readFileSync(new URL('../qt/app/js/extract.js', import.meta.url), 'utf8');
const context = vm.createContext({});
vm.runInContext(`${source}\nthis.campus = Campus;`, context);

function page({field = '', button = '', count = '12人已报名/限100人'} = {}) {
  const item = field && {
    querySelector(selector) {
      if (selector === '.main-title') return {textContent: '我的报名状态'};
      if (selector === '.main-value') return {textContent: field};
    }
  };
  return {
    querySelectorAll(selector) {
      if (selector === '.main-item') return item ? [item] : [];
      if (selector.startsWith('button')) return button ? [{textContent: button, getClientRects: () => [1]}] : [];
      return [];
    },
    // This aggregate count is deliberately not consulted by personalRegistration.
    querySelector: () => ({textContent: count})
  };
}

assert.equal(context.campus.personalRegistration(page()), 'unknown');
assert.equal(context.campus.personalRegistration(page({field: '报名成功'})), 'registered');
assert.equal(context.campus.personalRegistration(page({field: '审核中'})), 'unknown');
assert.equal(context.campus.personalRegistration(page({button: '取消报名'})), 'registered');
assert.equal(context.campus.personalRegistration(page({button: '立即报名'})), 'not_registered');
assert.equal(context.campus.personalRegistration(page({field: '未报名'})), 'not_registered');
assert.equal(context.campus.personalRegistration(page({button: '12人已报名'})), 'unknown');
console.log('Personal registration extraction passed');

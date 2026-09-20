import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import vm from 'node:vm';
vm.runInThisContext(await fs.readFile('extension/core.js','utf8'));
const C=globalThis.Campus;
const base={name:'测试活动',registration:'2026/09/21 10:00 —— 2026/09/21 12:00',remaining:'2'};
test('时间采用北京时间，正确判断边界、名额和失败',()=>{
  assert.equal(C.timestamp('2026/09/21 10:00'),Date.parse('2026-09-21T02:00:00Z'));
  assert.equal(C.status(base,C.timestamp('2026/09/21 09:59')),'报名未开始');
  assert.equal(C.status(base,C.timestamp('2026/09/21 10:00')),'报名时间内');
  assert.equal(C.status({...base,remaining:'0'},C.timestamp('2026/09/21 11:00')),'名额已满');
  assert.equal(C.status(base,C.timestamp('2026/09/21 12:00')),'报名已截止');
  assert.equal(C.status({...base,registration:'——'}),'时间待确认');
  assert.equal(C.status({...base,error:'读取超时'}),'读取失败');
});
test('缺失时间放最后，同名不同活动不丢失',()=>{
  const list=[{...base,id:'b',registration:'2026/09/22 10:00 —— 2026/09/22 12:00'},{...base,id:'c',registration:''},{...base,id:'a'}];
  assert.deepEqual(C.sorted(list).map(r=>r.id),['a','b','c']);
});
test('CSV 防公式注入、引号转义、BOM 和链接白名单',()=>{
  const result=C.csv([{...base,name:'=1+1,"测试"\n第二行',url:'javascript:alert(1)'}]);
  assert.ok(result.startsWith('\uFEFF'));
  assert.ok(result.includes('"\'=1+1,""测试""\n第二行"'));
  assert.ok(!result.includes('javascript:'));
});
test('初始数据完整、唯一、可解析，两个同名活动保留',async()=>{
  const rows=JSON.parse(await fs.readFile('activities.json','utf8'));
  assert.equal(rows.length,25);assert.equal(new Set(rows.map(r=>r.id)).size,25);
  assert.equal(rows.filter(r=>r.name==='中华人民共和国保守国家秘密法').length,2);
  for(const r of rows){assert.equal(Object.values(C.registrationDates(r)).filter(Boolean).length,2);assert.ok(r.activityTime && r.place && r.capacity);if(r.capacity!=='不限')assert.equal(Number(r.capacity)-Number(r.registered),Number(r.remaining));}
});
test('离线快照包含完整可重新初始化的脚本',()=>{
  const html=C.snapshot([base]);assert.ok(html.includes('function campusFactory'));assert.ok(html.includes('Campus.mount'));
  const script=html.match(/<script>([\s\S]*)<\/script>/)[1];new vm.Script(script);
});

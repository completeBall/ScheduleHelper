(function campusFactory(scope) {
  'use strict';
  const SCHOOL = 'https://study.gdipu.edu.cn';
  const normalize = v => String(v ?? '').replace(/\s+/g, ' ').trim();
  const dateMatches = text => normalize(text).match(/\d{4}[/-]\d{1,2}[/-]\d{1,2}\s+\d{1,2}:\d{2}/g) || [];
  function timestamp(text) {
    const m = String(text || '').match(/(\d{4})[/-](\d{1,2})[/-](\d{1,2})\s+(\d{1,2}):(\d{2})/);
    if (!m) return NaN;
    return Date.parse(`${m[1]}-${m[2].padStart(2,'0')}-${m[3].padStart(2,'0')}T${m[4].padStart(2,'0')}:${m[5]}:00+08:00`);
  }
  function registrationDates(row) { const d = dateMatches(row.registration); return {start:d[0] || '', end:d[1] || ''}; }
  function status(row, now = Date.now()) {
    if (row.error) return '读取失败';
    const {start,end} = registrationDates(row);
    if (!start || !end) return '时间待确认';
    if (now < timestamp(start)) return '报名未开始';
    if (now >= timestamp(end)) return '报名已截止';
    if (String(row.remaining) === '0') return '名额已满';
    return '报名时间内';
  }
  function sorted(rows) {
    return [...rows].sort((a,b) => (timestamp(registrationDates(a).start) || Infinity) - (timestamp(registrationDates(b).start) || Infinity) || a.name.localeCompare(b.name,'zh-CN'));
  }
  function safeURL(value) {
    try { const u = new URL(value); return u.origin === SCHOOL && u.pathname === '/CloudPortal/CloudActivityDetail' ? u.href : ''; } catch {return '';}
  }
  function csvCell(v) {
    let text = String(v ?? '');
    if (/^[\s]*[=+@-]/.test(text) || /^[\t\r]/.test(text)) text = "'" + text;
    return '"' + text.replace(/"/g,'""') + '"';
  }
  function csv(rows) {
    const headers = ['活动名称','报名开始时间','报名截止时间','活动地点','活动时间','活动总名额','剩余名额','已报名人数','报名状态（按时间判断）','活动类型','发起组织','活动链接','采集时间','读取说明'];
    return '\uFEFF' + [headers, ...sorted(rows).map(r => {const d = registrationDates(r); return [r.name,d.start,d.end,r.place,r.activityTime,r.capacity,r.remaining,r.registered,status(r),r.type,r.organizer,safeURL(r.url),r.collectedAt,r.error || ''];})].map(r=>r.map(csvCell).join(',')).join('\r\n');
  }
  function download(name, text, type) {
    const url = URL.createObjectURL(new Blob([text],{type}));
    const a = document.createElement('a'); a.href=url; a.download=name; a.click();
    setTimeout(()=>URL.revokeObjectURL(url),10000);
  }
  function extractCards(doc) {
    return [...doc.querySelectorAll('.table-container .card-list-item')].map((e,index) => ({
      name: normalize(e.querySelector('.a-name')?.getAttribute('title') || e.querySelector('.a-name')?.textContent),
      activityTime: normalize(e.querySelector('.a-time')?.getAttribute('title') || e.querySelector('.a-time')?.textContent),
      place: normalize(e.querySelector('.a-address [title]')?.getAttribute('title')),
      teacher: normalize(e.querySelector('.a-tea')?.getAttribute('title') || e.querySelector('.a-tea')?.textContent), index
    }));
  }
  function extractDetail(doc, row, url) {
    const fields = Object.fromEntries([...doc.querySelectorAll('.main-item')].map(e=>[normalize(e.querySelector('.main-title')?.textContent),normalize(e.querySelector('.main-value')?.textContent)]));
    const cap = normalize(doc.querySelector('.places-limit')?.textContent);
    const m = cap.match(/^(\d+)人已报名\s*\/\s*(?:不限|限?(\d+)人?)$/);
    const remaining = normalize(doc.querySelector('.places-rest')?.textContent).replace(/^剩余名额[：:]\s*/,'');
    if (dateMatches(fields['活动报名时间']).length !== 2 || !m || !/^(不限|\d+)$/.test(remaining)) throw new Error('报名时间或名额尚未加载，或页面字段已改变');
    return {...row, registration:fields['活动报名时间'], place: fields['活动场地'] && fields['活动场地'] !== '--' ? fields['活动场地'] : row.place,
      capacity:m[2] || '不限', remaining, registered:m[1], organizer:fields['活动发起组织'],type:fields['活动类型'],url,collectedAt:new Date().toISOString()};
  }
  const CSS = `
    :host{all:initial;display:block;min-width:0;color:#172b3a;font:14px/1.6 "Microsoft YaHei",system-ui,sans-serif;color-scheme:light}*{box-sizing:border-box}[hidden]{display:none!important}.top{flex-wrap:wrap}.shell{width:100%;min-width:0}
    .shell{background:#f4f7f9;min-height:100%;padding:30px 32px 24px}.top{display:flex;justify-content:space-between;gap:20px;align-items:center}.eyebrow{color:#16827d;font-size:12px;letter-spacing:2px;font-weight:700}h1{font-size:29px;line-height:1.4;margin:8px 0}p{margin:6px 0;color:#667986}button,a,input,select{font:inherit}button,.link-button{border:1px solid #d5e0e6;border-radius:8px;padding:9px 15px;background:white;color:#243c4b;cursor:pointer;text-decoration:none;display:inline-block}button:hover,.link-button:hover{background:#eef7f5}button.primary{background:#087d75;color:white;border-color:#087d75}button:disabled{opacity:.45;cursor:default}.actions{display:flex;gap:8px;flex-wrap:wrap}.summary{display:flex;gap:36px;padding:20px 0;border-bottom:1px solid #dce6ea;margin:12px 0 18px}.metric{font-size:27px;font-weight:700;color:#163c4b}.metric-label{font-size:12px;color:#667986}.toolbar{display:flex;align-items:center;gap:10px;margin:16px 0;flex-wrap:wrap}input,select{border:1px solid #d5e0e6;padding:10px 12px;background:white;border-radius:7px;color:#172b3a}input{min-width:250px;flex:1}select{max-width:230px}.count{color:#607580;font-size:12px;margin-left:auto}.table-wrap{overflow:auto;border:1px solid #dbe5ea;border-radius:10px;background:white;max-height:62vh}table{width:100%;border-collapse:collapse;font-size:13px;min-width:1120px}th{position:sticky;top:0;background:#edf3f6;color:#546b79;font-size:12px;z-index:1;text-align:left;white-space:nowrap}th,td{padding:14px 16px;border-bottom:1px solid #e8eef1;vertical-align:top}td:first-child{min-width:240px;max-width:360px}td.time{min-width:158px;white-space:nowrap}td.place{min-width:150px;max-width:240px}td.activity{min-width:165px}td.quota{min-width:105px}tbody tr:hover{background:#f8fbfc}a.name{font-weight:600;color:#183f50;text-decoration:none}a.name:hover{text-decoration:underline;color:#087d75}.sub{display:block;color:#758893;font-size:11px;margin-top:5px}.badge{display:inline-block;border-radius:4px;padding:3px 7px;background:#edf3f8;color:#4c6d89;font-size:11px;white-space:nowrap}.badge.open{background:#e5f5ee;color:#18764d}.badge.closed{background:#f0f1f2;color:#7a8186}.badge.error{background:#fbeae7;color:#a43d30}.quota b{font-size:17px;font-weight:650}.notice{background:#e9f3f1;border-left:3px solid #278e83;padding:10px 14px;margin:14px 0;color:#3e655f;font-size:12px}.progress{font-size:13px;color:#245c65;margin-top:12px}.empty{padding:40px;text-align:center;color:#758893}.foot{font-size:11px;color:#738894;margin-top:14px}.heading-extra{display:flex;gap:8px;align-items:center}.close{font-size:20px;padding:2px 12px}.start{color:#087d75;font-weight:600}.warning{color:#a43d30}.help{margin:16px 0;color:#526a77;font-size:13px}summary{cursor:pointer}ol{padding-left:22px}.help a{color:#087d75}.bookmark{background:#087d75;color:white!important;border-radius:7px;padding:8px 14px;display:inline-block;text-decoration:none;margin:6px 0}.no-wrap{white-space:nowrap}@media(max-width:800px){.shell{padding:18px}.top{align-items:flex-start;flex-direction:column}.summary{gap:22px}h1{font-size:24px}.table-wrap{max-height:65vh}}
  `;
  function mount(host, initial = [], options = {}) {
    const shadow = host.attachShadow({mode:'open'});
    shadow.innerHTML = `<style>${CSS}</style><main class="shell"><header class="top"><div><div class="eyebrow">GDIPU · 校园活动</div><h1>未开始活动汇总</h1><p>按报名开始时间排列，把值得关注的活动放在一起。</p></div><div class="heading-extra"><div class="actions"><button class="primary" id="refresh">重新采集</button><button id="export">导出 Excel</button><button id="save">保存网页</button></div><button class="close" id="close" aria-label="关闭">×</button></div></header><div class="summary"><div><div class="metric" id="total">0</div><div class="metric-label">未开始活动</div></div><div><div class="metric" id="soon">0</div><div class="metric-label">尚未开放报名</div></div><div><div class="metric" id="open">0</div><div class="metric-label">报名时间内</div></div><div><div class="metric" id="limited">0</div><div class="metric-label">有名额上限</div></div></div><div class="notice">已排除名称含“团日”“班会”及类型为“班级活动”的项目。报名状态按北京时间计算，名额为采集时数值。</div><div class="progress" id="progress" role="status"></div><div class="toolbar"><input id="query" aria-label="搜索活动" placeholder="搜索活动名称、地点或发起组织"><select id="status" aria-label="报名状态"><option value="">全部报名状态</option><option>报名未开始</option><option>报名时间内</option><option>报名已截止</option><option>名额已满</option><option>读取失败</option></select><select id="type" aria-label="活动类型"><option value="">全部活动类型</option></select><select id="sort" aria-label="排序方式"><option value="start">报名开始时间 ↑</option><option value="end">报名截止时间 ↑</option></select><span class="count" id="count"></span></div><div class="table-wrap"><table><thead><tr><th>活动名称 / 发起组织</th><th>报名开始</th><th>报名截止</th><th>活动地点</th><th>活动时间</th><th>总名额 / 剩余</th><th>报名状态</th></tr></thead><tbody id="rows"></tbody></table><div class="empty" id="empty">暂无活动</div></div><div class="foot" id="foot"></div><div id="help"></div></main>`;
    const $ = id => shadow.getElementById(id);
    let rows = initial.slice(), displayed = [], running = false;
    function textEl(tag, text, cls) { const e=document.createElement(tag); e.textContent=text; if(cls)e.className=cls; return e; }
    function render() {
      const query=$('query').value.trim().toLowerCase(), filter=$('status').value, type=$('type').value;
      $('total').textContent=rows.length;
      $('soon').textContent=rows.filter(r=>status(r)==='报名未开始').length;
      $('open').textContent=rows.filter(r=>status(r)==='报名时间内').length;
      $('limited').textContent=rows.filter(r=>/^\d+$/.test(String(r.capacity))).length;
      displayed=sorted(rows).filter(r=>(!query || [r.name,r.place,r.organizer].join(' ').toLowerCase().includes(query)) && (!filter || status(r)===filter) && (!type || r.type===type));
      if($('sort').value==='end')displayed.sort((a,b)=>(timestamp(registrationDates(a).end)||Infinity)-(timestamp(registrationDates(b).end)||Infinity));
      $('rows').replaceChildren();
      for (const r of displayed) {
        const tr=document.createElement('tr'), title=document.createElement('td'), link=textEl('a',r.name,'name'), url=safeURL(r.url);
        if(url){link.href=url;link.target='_blank';link.rel='noopener noreferrer';}
        title.append(link,textEl('span',`${r.type || '类型未提供'} · ${r.organizer || '组织未提供'}`,'sub'));
        if(r.error) title.append(textEl('span',r.error,'sub warning'));
        const d=registrationDates(r);
        const start=textEl('td',d.start || '未获取','time start'), end=textEl('td',d.end || '未获取','time');
        const place=textEl('td',r.place || '未提供','place'), activity=textEl('td',r.activityTime || '未提供','activity');
        const quota=textEl('td','','quota'); quota.append(textEl('b',r.capacity || '未获取'),textEl('span',`剩余 ${r.remaining ?? '未获取'}`,'sub'));
        const s=status(r), badge=textEl('td',''); badge.append(textEl('span',s,'badge '+(s==='报名时间内'?'open':s==='读取失败'?'error':s==='报名已截止'?'closed':'')));
        tr.append(title,start,end,place,activity,quota,badge); $('rows').append(tr);
      }
      $('count').textContent=`显示 ${displayed.length} / ${rows.length} 项`;
      $('empty').hidden=displayed.length>0;
      $('export').disabled=displayed.length===0;
      $('save').disabled=rows.length===0;
      const times=rows.map(r=>r.collectedAt).filter(Boolean).sort();
      $('foot').textContent=times.length?`数据采集于 ${new Date(times[0]).toLocaleString('zh-CN',{timeZone:'Asia/Shanghai',hour12:false})} 起 · 北京时间 · 导出包含当前筛选结果，按报名开始时间升序。`:'数据仅在本机浏览器中处理。';
    }
    function setRows(value) {rows=value.slice();const chosen=$('type').value;$('type').replaceChildren(new Option('全部活动类型',''));for(const type of [...new Set(rows.map(r=>r.type).filter(Boolean))].sort())$('type').append(new Option(type,type));$('type').value=chosen;render();}
    ['query','status','type','sort'].forEach(id=>$(id).addEventListener('input',render));
    $('export').onclick=()=>options.onExport?.(displayed);
    $('save').onclick=()=>download(`活动汇总_${new Date().toISOString().slice(0,10)}.html`,snapshot(rows),'text/html;charset=utf-8');
    $('close').hidden=!options.onClose; $('close').onclick=()=>options.onClose?.();
    $('refresh').textContent=options.refreshLabel || (options.onRefresh?'重新采集':'如何获取最新活动');
    $('refresh').onclick=()=>options.onRefresh ? options.onRefresh() : $('help').scrollIntoView({behavior:'smooth'});
    setRows(rows);
    const timer=setInterval(()=>{if(!host.isConnected){clearInterval(timer);return;}if(!running)render();},60000);
    return {setRows,shadow,progress(message){$('progress').textContent=message;},busy(value){running=value;$('refresh').disabled=value;},helpHTML(html){$('help').innerHTML=html;}};
  }
  function snapshot(rows) {
    const code = '(' + factorySource + ')(globalThis);';
    const data=JSON.stringify(rows).replace(/</g,'\\u003c');
    return '<!doctype html><html lang="zh-CN"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>广轻未开始活动汇总</title><style>body{margin:0;background:#f4f7f9}</style><div id="app"></div><script>'+code.replace(/<\/script/gi,'<\\/script')+'\nCampus.mount(document.getElementById("app"),'+data+',{refreshLabel:"打开学校活动广场",onRefresh:()=>window.open("https://study.gdipu.edu.cn/CloudPortal/CloudSquare","_blank","noopener")});<\/script></html>';
  }
  const factorySource = campusFactory.toString();
  scope.Campus={normalize,timestamp,registrationDates,status,sorted,safeURL,csv,download,extractCards,extractDetail,mount,snapshot,CSS,SCHOOL};
})(typeof globalThis !== 'undefined' ? globalThis : this);

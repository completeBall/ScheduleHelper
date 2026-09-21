// DOM extraction helpers injected into the school / academic pages.
// DOM selectors and parsing rules used by the embedded school and academic pages.
const Campus = (() => {
  const normalize = v => String(v ?? '').replace(/\s+/g, ' ').trim();
  const dateMatches = text => normalize(text).match(/\d{4}[/-]\d{1,2}[/-]\d{1,2}\s+\d{1,2}:\d{2}/g) || [];
  function extractCards(doc) {
    return [...doc.querySelectorAll('.table-container .card-list-item')].map((e, index) => ({
      name: normalize(e.querySelector('.a-name')?.getAttribute('title') || e.querySelector('.a-name')?.textContent),
      activityTime: normalize(e.querySelector('.a-time')?.getAttribute('title') || e.querySelector('.a-time')?.textContent),
      place: normalize(e.querySelector('.a-address [title]')?.getAttribute('title')),
      teacher: normalize(e.querySelector('.a-tea')?.getAttribute('title') || e.querySelector('.a-tea')?.textContent), index
    }));
  }
  function extractDetail(doc, row, url) {
    const fields = Object.fromEntries([...doc.querySelectorAll('.main-item')].map(e => [normalize(e.querySelector('.main-title')?.textContent), normalize(e.querySelector('.main-value')?.textContent)]));
    const cap = normalize(doc.querySelector('.places-limit')?.textContent);
    const m = cap.match(/^(\d+)人已报名\s*\/\s*(?:不限|限?(\d+)人?)$/);
    const remaining = normalize(doc.querySelector('.places-rest')?.textContent).replace(/^剩余名额[：:]\s*/, '');
    if (dateMatches(fields['活动报名时间']).length !== 2 || !m || !/^(不限|\d+)$/.test(remaining)) throw new Error('报名时间或名额尚未加载，或页面字段已改变');
    return {...row, registration: fields['活动报名时间'], place: fields['活动场地'] && fields['活动场地'] !== '--' ? fields['活动场地'] : row.place,
      capacity: m[2] || '不限', remaining, registered: m[1], organizer: fields['活动发起组织'], type: fields['活动类型'], url, collectedAt: new Date().toISOString()};
  }
  return {normalize, extractCards, extractDetail};
})();

const GdipuTimetable = (() => {
  const clean = s => String(s || '').replace(/ /g, ' ').replace(/[ \t]+/g, ' ').trim();
  function weekNumbers(raw) {
    const result = new Set(); let found = false;
    const pattern = /([\d\s,，、\-－~～至]+)\s*(?:\(\s*([单双])?\s*周\s*\)|（\s*([单双])?\s*周\s*）|周(?:\s*[（(]?([单双])(?:周)?[）)]?)?)/g;
    for (const m of raw.matchAll(pattern)) {
      found = true; const parity = m[2] || m[3] || m[4] || (/单周/.test(m[0]) ? '单' : /双周/.test(m[0]) ? '双' : '');
      for (const item of m[1].replace(/\s/g, '').split(/[,，、]/)) {
        const n = item.match(/^(\d+)(?:[-－~～至](\d+))?$/); if (!n) continue;
        const start = +n[1], end = n[2] ? +n[2] : start; if (start < 1 || end > 40 || end < start) continue;
        for (let w = start; w <= end; w++) if (!parity || (parity === '单' ? w % 2 === 1 : w % 2 === 0)) result.add(w);
      }
    }
    return found && result.size ? [...result].sort((a, b) => a - b) : null;
  }
  function slotsFrom(text) {
    const s = clean(text).replace(/\d{1,2}:\d{2}\s*[-–—~～]\s*\d{1,2}:\d{2}/g, '');
    const big = s.match(/第([一二三四五六1-6])大节/); if (big) { const block = /\d/.test(big[1]) ? +big[1] : '一二三四五六'.indexOf(big[1]) + 1; return [block * 2 - 1, block * 2]; }
    const m = s.match(/(?:第|\[|【)?\s*(\d{1,2})\s*(?:[,，、\-－~～至]\s*(\d{1,2}))?\s*(?:[\]】]?\s*节|[\]】])/);
    if (!m) { const simple = s.match(/^\s*(\d{1,2})(?:\s*[-－~～、,]\s*(\d{1,2}))?\s*$/); if (!simple) return []; const a = +simple[1], b = +(simple[2] || simple[1]); return a >= 1 && b <= 12 && b >= a ? Array.from({length: b - a + 1}, (_, i) => a + i) : []; }
    const a = +m[1], b = +(m[2] || m[1]); if (a < 1 || b > 12 || b < a) return [];
    return Array.from({length: b - a + 1}, (_, i) => a + i);
  }
  function gridOf(table) {
    const grid = []; [...table.rows].forEach((row, y) => { grid[y] ||= []; let x = 0; for (const cell of row.cells) { while (grid[y][x]) x++; for (let dy = 0; dy < (cell.rowSpan || 1); dy++) { grid[y + dy] ||= []; for (let dx = 0; dx < (cell.colSpan || 1); dx++) grid[y + dy][x + dx] = cell; } x += cell.colSpan || 1; } }); return grid;
  }
  function visible(el) { const s = el.ownerDocument.defaultView.getComputedStyle(el); return s.display !== 'none' && s.visibility !== 'hidden'; }
  function readDocument(doc) {
    const candidates = [];
    for (const table of doc.querySelectorAll('table')) {
      if (!visible(table)) continue;
      const grid = gridOf(table); let header = -1, columns = [];
      for (let y = 0; y < Math.min(grid.length, 10); y++) {
        const seen = new Set(), cols = [];
        grid[y].forEach((cell, x) => { const t = clean(cell.innerText); const m = t.match(/^(?:星期|周)([一二三四五六日天])$/); if (m) { const d = '一二三四五六日'.indexOf(m[1] === '天' ? '日' : m[1]) + 1; if (!seen.has(d)) { seen.add(d); cols.push({x, day: d}); } } });
        if (cols.length >= 5) { header = y; columns = cols; break; }
      }
      if (header < 0) continue;
      const firstDay = Math.min(...columns.map(c => c.x)), mapping = new Map(); let unknownRows = 0;
      for (let y = header + 1; y < grid.length; y++) {
        const labels = [...new Set(grid[y].slice(0, firstDay))].map(c => clean(c.innerText)).join(' '), slots = slotsFrom(labels);
        for (const col of columns) {
          const cell = grid[y][col.x]; if (!cell || !visible(cell)) continue;
          const raw = clean(cell.innerText); if (!raw || /^[-—\s]+$/.test(raw)) continue;
          if (!slots.length) { if (/^备注\s*[:：]?/.test(labels)) continue; unknownRows++; continue; }
          const key = col.day + ':' + (cell.cellIndex) + ':' + cell.parentElement.rowIndex;
          if (!mapping.has(key)) mapping.set(key, {cell, day: col.day, slots: new Set()});
          slots.forEach(n => mapping.get(key).slots.add(n));
        }
      }
      const courses = [];
      for (const entry of mapping.values()) {
        let blocks = [...entry.cell.querySelectorAll('.kbcontent')].filter(visible);
        if (!blocks.length) blocks = [entry.cell];
        for (const block of blocks) {
          const segments = block.innerText.split(/(?:\n\s*)?[-─━]{4,}(?:\s*\n)?/).map(clean).filter(Boolean);
          for (const raw of segments) {
            if (!raw || /^[-—\s]+$/.test(raw)) continue;
            const lines = raw.split(/\n/).map(clean).filter(Boolean);
            const name = lines.find(s => !/^\d{1,2}:\d{2}/.test(s)) || lines[0];
            courses.push({name, raw, day: entry.day, slots: [...entry.slots].sort((a, b) => a - b), weeks: weekNumbers(raw)});
          }
        }
      }
      candidates.push({courses, unknownRows, recognized: true});
    }
    const chosen = candidates.sort((a, b) => b.courses.length - a.courses.length)[0]; if (!chosen) return null;
    const semester = [...doc.querySelectorAll('select')].map(s => s.selectedOptions?.[0]?.textContent || '').map(clean).find(s => /20\d{2}/.test(s) && (/学期|20\d{2}.*20\d{2}/.test(s))) || '';
    return {...chosen, semester, source: doc.URL};
  }
  function documents(root) { const result = []; function visit(d) { if (!d || result.includes(d)) return; result.push(d); for (const f of d.querySelectorAll('iframe,frame')) try { visit(f.contentDocument); } catch {} } visit(root); return result; }
  function extract(root) { const found = documents(root).map(readDocument).filter(Boolean).sort((a, b) => b.courses.length - a.courses.length); return found[0] || null; }
  function nextLink(root) {
    const labels = ['学生个人课表', '个人课表', '我的课表', '学期理论课表', '理论课表', '课表查询'];
    for (const label of labels) for (const d of documents(root)) for (const e of d.querySelectorAll('a,button,[role=button]')) if (visible(e) && clean(e.innerText) === label) return {element: e, label};
    return null;
  }
  return {extract, nextLink};
})();

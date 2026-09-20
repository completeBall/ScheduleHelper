(function timetableFactory(scope){
  'use strict';
  const periods=[['上午','第1、2节','08:30–09:55'],['上午','第3、4节','10:15–11:40'],['下午','第5、6节','14:00–15:25'],['下午','第7、8节','15:45–17:10'],['晚上','第9、10节','18:30–19:55'],['晚上','第11、12节','20:00–21:25']];
  const days=['星期一','星期二','星期三','星期四','星期五','星期六','星期日'];
  const clean=s=>String(s||'').replace(/\u00a0/g,' ').replace(/[ \t]+/g,' ').trim();
  function weekNumbers(raw){
    const result=new Set();let found=false;
    const pattern=/([\d\s,，、\-－~～至]+)\s*(?:\(\s*([单双])?\s*周\s*\)|（\s*([单双])?\s*周\s*）|周(?:\s*[（(]?([单双])(?:周)?[）)]?)?)/g;
    for(const m of raw.matchAll(pattern)){
      found=true;const parity=m[2]||m[3]||m[4]||(/单周/.test(m[0])?'单':/双周/.test(m[0])?'双':'');
      for(const item of m[1].replace(/\s/g,'').split(/[,，、]/)){
        const n=item.match(/^(\d+)(?:[-－~～至](\d+))?$/);if(!n)continue;
        const start=+n[1],end=n[2]?+n[2]:start;if(start<1||end>40||end<start)continue;
        for(let w=start;w<=end;w++)if(!parity||(parity==='单'?w%2===1:w%2===0))result.add(w);
      }
    }
    return found&&result.size?[...result].sort((a,b)=>a-b):null;
  }
  function slotsFrom(text){
    const s=clean(text).replace(/\d{1,2}:\d{2}\s*[-–—~～]\s*\d{1,2}:\d{2}/g,'');
    const big=s.match(/第([一二三四五六1-6])大节/);if(big){const block=/\d/.test(big[1])?+big[1]:'一二三四五六'.indexOf(big[1])+1;return [block*2-1,block*2];}
    const m=s.match(/(?:第|\[|【)?\s*(\d{1,2})\s*(?:[,，、\-－~～至]\s*(\d{1,2}))?\s*(?:[\]】]?\s*节|[\]】])/);
    if(!m){const simple=s.match(/^\s*(\d{1,2})(?:\s*[-－~～、,]\s*(\d{1,2}))?\s*$/);if(!simple)return [];const a=+simple[1],b=+(simple[2]||simple[1]);return a>=1&&b<=12&&b>=a?Array.from({length:b-a+1},(_,i)=>a+i):[];}
    const a=+m[1],b=+(m[2]||m[1]);if(a<1||b>12||b<a)return [];
    return Array.from({length:b-a+1},(_,i)=>a+i);
  }
  function gridOf(table){
    const grid=[];[...table.rows].forEach((row,y)=>{grid[y]||=[];let x=0;for(const cell of row.cells){while(grid[y][x])x++;for(let dy=0;dy<(cell.rowSpan||1);dy++){grid[y+dy]||=[];for(let dx=0;dx<(cell.colSpan||1);dx++)grid[y+dy][x+dx]=cell;}x+=cell.colSpan||1;}});return grid;
  }
  function visible(el){const s=el.ownerDocument.defaultView.getComputedStyle(el);return s.display!=='none'&&s.visibility!=='hidden';}
  function readDocument(doc){
    const candidates=[];
    for(const table of doc.querySelectorAll('table')){
      if(!visible(table))continue;
      const grid=gridOf(table);let header=-1,columns=[];
      for(let y=0;y<Math.min(grid.length,10);y++){
        const seen=new Set(),cols=[];
        grid[y].forEach((cell,x)=>{const t=clean(cell.innerText);const m=t.match(/^(?:星期|周)([一二三四五六日天])$/);if(m){const d='一二三四五六日'.indexOf(m[1]==='天'?'日':m[1])+1;if(!seen.has(d)){seen.add(d);cols.push({x,day:d});}}});
        if(cols.length>=5){header=y;columns=cols;break;}
      }
      if(header<0)continue;
      const firstDay=Math.min(...columns.map(c=>c.x)),mapping=new Map();let unknownRows=0;
      for(let y=header+1;y<grid.length;y++){
        const labels=[...new Set(grid[y].slice(0,firstDay))].map(c=>clean(c.innerText)).join(' '),slots=slotsFrom(labels);
        for(const col of columns){
          const cell=grid[y][col.x];if(!cell||!visible(cell))continue;
          const raw=clean(cell.innerText);if(!raw||/^[-—\s]+$/.test(raw))continue;
          if(!slots.length){if(/^备注\s*[:：]?/.test(labels))continue;unknownRows++;continue;}
          const key=col.day+':'+(cell.cellIndex)+':'+cell.parentElement.rowIndex;
          if(!mapping.has(key))mapping.set(key,{cell,day:col.day,slots:new Set()});
          slots.forEach(n=>mapping.get(key).slots.add(n));
        }
      }
      const courses=[];
      for(const entry of mapping.values()){
        let blocks=[...entry.cell.querySelectorAll('.kbcontent')].filter(visible);
        if(!blocks.length)blocks=[entry.cell];
        for(const block of blocks){
          const segments=block.innerText.split(/(?:\n\s*)?[-─━]{4,}(?:\s*\n)?/).map(clean).filter(Boolean);
          for(const raw of segments){
            if(!raw||/^[-—\s]+$/.test(raw))continue;
            const lines=raw.split(/\n/).map(clean).filter(Boolean);
            const name=lines.find(s=>!/^\d{1,2}:\d{2}/.test(s))||lines[0];
            courses.push({name,raw,day:entry.day,slots:[...entry.slots].sort((a,b)=>a-b),weeks:weekNumbers(raw)});
          }
        }
      }
      candidates.push({courses,unknownRows,recognized:true});
    }
    const chosen=candidates.sort((a,b)=>b.courses.length-a.courses.length)[0];if(!chosen)return null;
    const semester=[...doc.querySelectorAll('select')].map(s=>s.selectedOptions?.[0]?.textContent||'').map(clean).find(s=>/20\d{2}/.test(s)&&(/学期|20\d{2}.*20\d{2}/.test(s)))||'';
    return {...chosen,semester,source:doc.URL};
  }
  function documents(root){const result=[];function visit(d){if(!d||result.includes(d))return;result.push(d);for(const f of d.querySelectorAll('iframe,frame'))try{visit(f.contentDocument);}catch{}}visit(root);return result;}
  function extract(root){const found=documents(root).map(readDocument).filter(Boolean).sort((a,b)=>b.courses.length-a.courses.length);return found[0]||null;}
  function nextLink(root){
    const labels=['学生个人课表','个人课表','我的课表','学期理论课表','理论课表','课表查询'];
    for(const label of labels)for(const d of documents(root))for(const e of d.querySelectorAll('a,button,[role=button]'))if(visible(e)&&clean(e.innerText)===label)return {element:e,label};
    return null;
  }
  function parseDate(value){const m=String(value||'').match(/^(\d{4})-(\d{2})-(\d{2})$/);if(!m)return null;const d=new Date(+m[1],+m[2]-1,+m[3]);return d.getFullYear()===+m[1]&&d.getMonth()===+m[2]-1&&d.getDate()===+m[3]?d:null;}
  function dateText(date){return [date.getFullYear(),String(date.getMonth()+1).padStart(2,'0'),String(date.getDate()).padStart(2,'0')].join('-');}
  function addDays(date,count){const result=new Date(date);result.setDate(result.getDate()+count);return result;}
  function weekOneMonday(referenceWeek,referenceDay,referenceDate){const date=parseDate(referenceDate);if(!date||referenceWeek<1||referenceWeek>40||referenceDay<1||referenceDay>7)return '';return dateText(addDays(date,-((referenceWeek-1)*7+referenceDay-1)));}
  function dateFor(anchor,week,day){const monday=parseDate(anchor);if(!monday||week<1||day<1||day>7)return '';return dateText(addDays(monday,(week-1)*7+day-1));}
  function activityReminder(row,anchor){
    const match=String(row?.registration||'').match(/(\d{4})[\/-](\d{1,2})[\/-](\d{1,2})\s+(\d{1,2}):(\d{2})/),monday=parseDate(anchor);if(!match||!monday)return null;
    const date=new Date(+match[1],+match[2]-1,+match[3]),diff=Math.round((Date.UTC(date.getFullYear(),date.getMonth(),date.getDate())-Date.UTC(monday.getFullYear(),monday.getMonth(),monday.getDate()))/86400000),week=Math.floor(diff/7)+1,day=((diff%7)+7)%7+1;if(week<1||week>40)return null;
    const minute=+match[4]*60+(+match[5]),starts=[10*60+15,14*60,15*60+45,18*60+30,20*60];let block=0;for(const start of starts)if(minute>=start)block++;
    return {row,week,day,block,start:`${match[1]}-${String(match[2]).padStart(2,'0')}-${String(match[3]).padStart(2,'0')} ${String(match[4]).padStart(2,'0')}:${match[5]}`};
  }
  function activityMoments(row,anchor){const registration=activityReminder(row,anchor);const event=activityReminder({...row,registration:row.activityTime},anchor);return [registration&&{...registration,kind:'registration'},event&&{...event,row,kind:'event'}].filter(Boolean);}
  function mount(host,initial,options={}){
    const shadow=host.attachShadow({mode:'open'});let data=initial||{courses:[]},activities=[];
    shadow.innerHTML=`<style>:host{display:block;font:14px/1.5 'Microsoft YaHei UI',system-ui;color:#233e50}*{box-sizing:border-box}[hidden]{display:none!important}main{padding:28px;background:#f5f8fb;min-height:100vh}header{display:flex;justify-content:space-between;align-items:center;gap:16px;flex-wrap:wrap}h1{font-size:28px;margin:5px 0}p{color:#728593;margin:5px 0}.eyebrow{font-size:12px;letter-spacing:2px;color:#138980;font-weight:bold}button,select,input{font:inherit;border:1px solid #d5e2e8;border-radius:8px;padding:10px 15px;background:white;color:#264b60}button{cursor:pointer}.primary{background:#168980;color:white;border-color:#168980}.actions,.tools,.dialog-actions{display:flex;gap:8px;flex-wrap:wrap;align-items:center}.tools{gap:14px;margin:22px 0 12px}.status{color:#23746f;background:#eaf5f2;padding:11px 15px;border-left:3px solid #168980;margin:14px 0}.wrap{overflow:auto;background:white;border:1px solid #dee7ee;border-radius:12px}table{border-collapse:collapse;width:100%;min-width:1080px;table-layout:fixed}th,td{border:1px solid #e6ecf1;padding:8px;vertical-align:top}th{background:#eaf2fb;color:#526f86;text-align:center;height:54px}.day-date{display:block;font-size:11px;color:#168980;margin-top:2px}.group{width:55px;background:#f5f8fb;text-align:center;vertical-align:middle;color:#7a8b98}.slot{width:125px;text-align:center;color:#607c8d;font-size:12px;vertical-align:middle;background:#fbfcfe}.slot span{display:block;margin-top:5px}.course-cell{height:120px}.course{position:relative;border-radius:7px;padding:9px;margin-bottom:6px;background:var(--bg);border-left:3px solid var(--accent);overflow-wrap:anywhere}.course b{display:block;font-size:13px;margin-bottom:5px;padding-right:20px}.course small{display:block;white-space:pre-line;font-size:11px;line-height:1.6;color:#426478}.manual-tag{font-size:10px;color:#966819}.remove{position:absolute;right:24px;top:1px;border:0;background:transparent;padding:2px 6px;color:#936060;font-size:16px}.reminder-dot{position:absolute;right:7px;top:7px;width:11px;height:11px;min-width:0;padding:0;border:0;border-radius:50%;background:#e53c3c;box-shadow:0 0 0 2px #fff}.reminder-panel{margin-top:8px;padding-top:7px;border-top:1px dashed #e3aaa5}.reminder-item+ .reminder-item{margin-top:8px}.reminder-item strong,.reminder-item span{display:block}.reminder-item strong{color:#9b2828;font-size:12px}.reminder-item span{white-space:pre-line;font-size:10px;line-height:1.55;color:#684b4b}.activity-card{background:#fff3f1!important}.note{font-size:12px;color:#758996;margin:12px 0}.empty{padding:12px 0;color:#668494}.week-unknown{font-size:10px;color:#966819}#meta{font-size:12px;color:#758996}dialog{border:0;border-radius:14px;padding:0;box-shadow:0 18px 60px #24445b44;width:min(470px,calc(100vw - 40px))}dialog::backdrop{background:#20374666}.dialog-body{padding:24px}.dialog-body h2{margin:0 0 16px}.form-grid{display:grid;grid-template-columns:1fr 1fr;gap:13px}.form-grid label{display:flex;flex-direction:column;gap:5px;color:#526f86}.wide{grid-column:1/-1}.dialog-actions{justify-content:flex-end;margin-top:20px}.hint{font-size:12px;color:#758996}</style><main><header><div><div class='eyebrow'>GDIPU · 课程安排</div><h1>我的课表</h1><p>从教务系统导入，也可手动补充临时调课。</p></div><div class='actions'><button id='login'>教务登录</button><button id='calendar'>设置日期</button><button id='add'>新增课程</button><button class='primary' id='import'>一键导入课表</button></div></header><div class='tools'><label>显示周次 <select id='week'><option value=''>全部周次</option>${Array.from({length:30},(_,i)=>`<option value='${i+1}'>第 ${i+1} 周</option>`).join('')}</select></label><span id='semester'></span><span id='count'></span></div><div id='status' class='status' role='status'>首次请先登录教务系统，再一键导入。导入后可离线查看。</div><div class='wrap'><table><thead><tr><th class='group'>时间</th><th class='slot'>节次</th>${days.map((d,i)=>`<th><span>${d}</span><span class='day-date' id='day${i+1}'></span></th>`).join('')}</tr></thead><tbody id='body'></tbody></table></div><p class='empty' id='empty'>尚未导入课程。上课时间已按你提供的课表设置。</p><p class='note'>请先“设置日期”并切换到具体周次。橙色卡片提醒报名开始，蓝色卡片提醒活动开始；即使同一时段有课程也会独立显示。</p><div id='meta'></div></main><dialog id='calendarDialog'><form method='dialog' class='dialog-body'><h2>设置课表日期</h2><div class='form-grid'><label>第几周<input id='refWeek' type='number' min='1' max='40' required></label><label>星期<select id='refDay'>${days.map((d,i)=>`<option value='${i+1}'>${d}</option>`).join('')}</select></label><label class='wide'>这一天的日期<input id='refDate' type='date' required></label></div><p class='hint'>只需设置任意一周的任意一天，其他周和日期会自动补全。</p><div class='dialog-actions'><button value='cancel'>取消</button><button id='saveDate' value='default' class='primary'>保存日期</button></div></form></dialog><dialog id='courseDialog'><form method='dialog' class='dialog-body'><h2>新增课程</h2><div class='form-grid'><label class='wide'>课程名称<input id='courseName' maxlength='80' required></label><label>教师<input id='teacher' maxlength='50'></label><label>地点<input id='place' maxlength='80'></label><label>星期<select id='courseDay'>${days.map((d,i)=>`<option value='${i+1}'>${d}</option>`).join('')}</select></label><label>节次<select id='courseSlot'>${periods.map((p,i)=>`<option value='${i}'>${p[1]}　${p[2]}</option>`).join('')}</select></label><label class='wide'>上课周次<input id='courseWeeks' placeholder='例如：2 或 2-6 或 1-8单周' required></label></div><div class='dialog-actions'><button value='cancel'>取消</button><button id='saveCourse' value='default' class='primary'>新增课程</button></div></form></dialog>`;
    shadow.querySelector('.note').textContent='报名开始提醒使用橙色卡片，活动开始提醒使用蓝色卡片；两种提醒独立显示，不包含报名截止时间。';
    const $=id=>shadow.getElementById(id),palettes=[['#eef3fe','#7796d9'],['#e8f5f1','#58a58e'],['#fff3e9','#d39d64'],['#f2edfa','#9c80c2'],['#eaf5fc','#6dacc9']];
    function save(){options.save?.(data);}
    function appendReminder(cell,item){const registration=item.kind==='registration',card=document.createElement('div');card.className='course reminder-card '+(registration?'registration-card':'event-card');card.style.setProperty('--bg',registration?'#fff4e8':'#eef3ff');card.style.setProperty('--accent',registration?'#e58a2f':'#527bd2');const title=document.createElement('b');title.textContent=registration?'活动报名提醒':'活动开始提醒';const detail=document.createElement('small');detail.textContent=[item.row.name,(registration?'报名时间：':'活动时间：')+item.start,item.row.place&&'地点：'+item.row.place].filter(Boolean).join('\n');card.append(title,detail);cell.append(card);}
    function render(){
      data.courses=data.courses||[];data.manualCourses=data.manualCourses||[];const week=+$('week').value,reminders=week&&data.weekOneMonday?activities.flatMap(a=>activityMoments(a,data.weekOneMonday)).filter(r=>r.week===week):[];const all=[...data.courses,...data.manualCourses],courses=all.filter(c=>!week||!c.weeks||c.weeks.includes(week));$('body').replaceChildren();
      for(let day=1;day<=7;day++){$('day'+day).textContent=week&&data.weekOneMonday?dateFor(data.weekOneMonday,week,day).slice(5).replace('-','/'):'';}
      for(let i=0;i<6;i++){
        const tr=document.createElement('tr');if(i%2===0){const td=document.createElement('td');td.className='group';td.rowSpan=2;td.textContent=periods[i][0];tr.append(td);}
        const slot=document.createElement('td');slot.className='slot';slot.textContent=periods[i][1];const time=document.createElement('span');time.textContent=periods[i][2];slot.append(time);tr.append(slot);
        for(let day=1;day<=7;day++){
          const cell=document.createElement('td');cell.className='course-cell';
          const cellCourses=courses.filter(c=>c.day===day&&c.slots.some(n=>Math.ceil(n/2)===i+1)),cellReminders=reminders.filter(r=>r.day===day&&r.block===i);
          for(const course of cellCourses){
            const card=document.createElement('div');card.className='course';let hash=0;for(const c of course.name)hash=(hash*31+c.charCodeAt(0))>>>0;const colors=palettes[hash%palettes.length];card.style.setProperty('--bg',colors[0]);card.style.setProperty('--accent',colors[1]);const title=document.createElement('b');title.textContent=course.name;const desc=document.createElement('small');desc.textContent=course.raw.startsWith(course.name)?course.raw.slice(course.name.length).trim():course.raw;card.append(title,desc);if(course.manual){const tag=document.createElement('span');tag.className='manual-tag';tag.textContent='手动添加';const remove=document.createElement('button');remove.className='remove';remove.type='button';remove.title='删除这条手动课程';remove.textContent='×';remove.onclick=()=>{if(confirm('删除手动课程“'+course.name+'”？')){data.manualCourses=data.manualCourses.filter(c=>c.id!==course.id);save();render();}};card.append(tag,remove);}else if(!course.weeks){const tag=document.createElement('span');tag.className='week-unknown';tag.textContent='周次未识别';card.append(tag);}cell.append(card);
          }
          for(const reminder of cellReminders)appendReminder(cell,reminder);
          tr.append(cell);
        }$('body').append(tr);
      }
      $('empty').hidden=courses.length>0||reminders.length>0;$('empty').textContent=all.length?'所选周次没有课程或活动提醒。':'尚未导入课程。上课时间已按你提供的课表设置。';$('semester').textContent=data.semester?'学期：'+data.semester:'';$('count').textContent=`显示 ${courses.length} 条课程安排${reminders.length?' · '+reminders.length+' 个报名提醒':''}`;$('meta').textContent=data.importedAt?'上次导入：'+new Date(data.importedAt).toLocaleString('zh-CN',{timeZone:'Asia/Shanghai',hour12:false}):'';
    }
    $('week').value=data.selectedWeek||'';$('week').onchange=()=>{data.selectedWeek=$('week').value;save();render();};$('login').onclick=()=>options.login?.();$('import').onclick=()=>options.import?.();
    $('calendar').onclick=()=>{$('refWeek').value=+$('week').value||1;$('refDay').value=1;const existing=data.weekOneMonday&&dateFor(data.weekOneMonday,+$('refWeek').value,1);$('refDate').value=existing||dateText(new Date());$('calendarDialog').showModal();};
    $('saveDate').onclick=e=>{e.preventDefault();const anchor=weekOneMonday(+$('refWeek').value,+$('refDay').value,$('refDate').value);if(!anchor){alert('请填写有效的周次和日期。');return;}data.weekOneMonday=anchor;data.selectedWeek=String(+$('refWeek').value);$('week').value=data.selectedWeek;save();render();$('calendarDialog').close();};
    $('add').onclick=()=>{$('courseName').value='';$('teacher').value='';$('place').value='';$('courseDay').value=1;$('courseSlot').value=0;$('courseWeeks').value=$('week').value||'1';$('courseDialog').showModal();};
    $('saveCourse').onclick=e=>{e.preventDefault();const name=clean($('courseName').value),weekInput=clean($('courseWeeks').value),normalized=/[单双]\s*周?$/.test(weekInput)?weekInput.replace(/([单双])\s*周?$/,'($1周)'):/周/.test(weekInput)?weekInput:weekInput+'(周)',weeks=weekNumbers(normalized),block=+$('courseSlot').value;if(!name){alert('请填写课程名称。');return;}if(!weeks){alert('周次格式无法识别，请填写如 2、2-6 或 1-8单周。');return;}const teacher=clean($('teacher').value),place=clean($('place').value),details=[teacher,weekInput+'周',place].filter(Boolean);const course={id:'manual-'+Date.now()+'-'+Math.random().toString(36).slice(2),manual:true,name,raw:[name,...details].join('\n'),day:+$('courseDay').value,slots:[block*2+1,block*2+2],weeks};data.manualCourses.push(course);save();render();$('courseDialog').close();};render();
    return {setData(value){data=value||{};$('week').value=data.selectedWeek||'';render();},setActivities(value){activities=Array.isArray(value)?value:[];render();},progress(text){$('status').textContent=text;},busy(value){$('import').disabled=value;$('login').disabled=value;},shadow};
  }
  scope.GdipuTimetable={periods,days,weekNumbers,slotsFrom,weekOneMonday,dateFor,activityReminder,activityMoments,extract,nextLink,mount};
})(globalThis);

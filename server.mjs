import http from 'node:http';
import fs from 'node:fs/promises';
import path from 'node:path';
const root=process.cwd();
const data=JSON.parse(await fs.readFile('activities.json','utf8')).slice(0,6);
data[0].name='同名活动'; data[0].type='学院活动'; data[1].name='同名活动'; data[1].type='学院活动';
const esc=v=>String(v).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
function fixture(url){
  const page=Number(url.searchParams.get('page')||1), selected=url.searchParams.get('status')==='pending';
  if(url.pathname.endsWith('CloudSquare')){
    const rows=selected?data.slice((page-1)*2,page*2):data.slice(0,1);
    return `<meta charset="utf-8"><div class="tag-radio-item ${selected?'':'active'}">进行中</div><div class="tag-radio-item ${selected?'active':''}" onclick="location.href='?status=pending&page=1'">未开始</div><div class="table-container">${rows.map(r=>`<div class="card-list-item" onclick="location.href='/fixture/CloudPortal/CloudActivityDetail?wid=${r.id}&dataType=HD'"><div class="a-name" title="${esc(r.name)}">${esc(r.name)}</div><span class="a-tea" title="${r.id}">${r.id}</span><span class="a-time" title="${r.activityTime}">${r.activityTime}</span><div class="a-address"><span title="${esc(r.place)}">${esc(r.place)}</span></div></div>`).join('')}</div><div class="el-pagination"><span class="el-pagination__total">共 ${selected?data.length:1} 条</span><ul class="el-pager"><li class="active">${page}</li></ul><button class="btn-next" ${!selected||page>=Math.ceil(data.length/2)?'disabled':''} onclick="location.href='?status=pending&page=${page+1}'">下一页</button></div>`;
  }
  const r=data.find(r=>r.id===url.searchParams.get('wid'));if(!r)return '';
  return `<meta charset="utf-8"><div class="txt"><span title="${esc(r.name)}">${esc(r.name)}</span></div><p class="places-rest">剩余名额：${r.remaining}</p><p class="places-limit">${r.registered}人已报名/${r.capacity==='不限'?'不限':`限${r.capacity}人`}</p>${[['活动报名时间',r.registration],['活动场地',r.place],['活动发起组织',r.organizer],['活动类型',r.type]].map(([k,v])=>`<li class="main-item"><p class="main-title">${k}</p><p class="main-value">${esc(v)}</p></li>`).join('')}<button>${r.id===data[2].id?'取消报名':'立即报名'}</button>`;
}
const types={'.html':'text/html; charset=utf-8','.js':'text/javascript; charset=utf-8','.json':'application/json; charset=utf-8','.css':'text/css; charset=utf-8','.md':'text/plain; charset=utf-8'};
http.createServer(async(req,res)=>{
  try{
    const url=new URL(req.url,'http://127.0.0.1');
    if(url.pathname.startsWith('/fixture/CloudPortal/')){res.setHeader('Content-Type','text/html; charset=utf-8');res.end(fixture(url));return;}
    const filename=path.resolve(root,'.'+decodeURIComponent(url.pathname==='/'?'/活动汇总.html':url.pathname));
    if(filename!==root&&!filename.startsWith(root+path.sep)){res.writeHead(403);res.end();return;}
    const content=await fs.readFile(filename);res.setHeader('Content-Type',types[path.extname(filename)]||'application/octet-stream');res.end(content);
  }catch{res.writeHead(404);res.end('Not found');}
}).listen(8765,'127.0.0.1',()=>console.log('Preview: http://127.0.0.1:8765'));

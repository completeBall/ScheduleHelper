import fs from 'node:fs/promises';
const times=['08:30-09:55','10:15-11:40','14:00-15:25','15:45-17:10','18:30-19:55','20:00-21:25'];
const fixtures=[{row:0,day:0,raw:'测试大学英语<br>测试教师甲<br>1-16(周)[01-02节]<br>测试教室101'},{row:2,day:2,raw:'测试程序设计<br>测试教师乙<br>1-15(单周)[05-06节]<br>测试机房201'},{row:5,day:6,raw:'测试体育课程<br>测试教师丙<br>2-16(双周)[11-12节]<br>测试场地'}];
const html='<meta charset="utf-8"><select><option>2026-2027学年第一学期</option></select><h1>学生个人课表</h1><table><tr><th>时间</th><th>节次</th>'+['一','二','三','四','五','六','日'].map(d=>'<th>星期'+d+'</th>').join('')+'</tr>'+times.map((time,row)=>'<tr>'+(row%2===0?'<td rowspan="2">'+['上午','下午','晚上'][row/2]+'</td>':'')+'<td>第'+(row*2+1)+'、'+(row*2+2)+'节<br>'+time+'</td>'+Array.from({length:7},(_,day)=>'<td>'+fixtures.filter(f=>f.row===row&&f.day===day).map(f=>'<div class="kbcontent">'+f.raw+'</div><div class="kbcontent" style="display:none">不应导入的隐藏副本</div>').join('')+'</td>').join('')+'</tr>').join('')+'</table>';
await fs.writeFile('timetable-main.html','<meta charset="utf-8"><a href="/timetable-frame.html">学生个人课表</a>');
await fs.writeFile('timetable-frame.html','<meta charset="utf-8"><iframe style="width:100%;height:800px" src="/timetable-table.html"></iframe>');
await fs.writeFile('timetable-table.html',html);
await fs.writeFile('timetable-empty.html',html.replace(/<div class="kbcontent">[\s\S]*?<\/div>/g,''));
await fs.writeFile('timetable-login.html','<meta charset="utf-8"><h1>登录</h1><input type="password">');

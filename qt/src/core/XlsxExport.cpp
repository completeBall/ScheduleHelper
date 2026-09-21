#include "XlsxExport.h"

#include <QFile>
#include <QLocale>
#include <algorithm>
#include <private/qzipreader_p.h>
#include <private/qzipwriter_p.h>

namespace Campus {

namespace {

QString xmlEscape(const QString &value)
{
    QString out;
    out.reserve(value.size());
    for (const QChar c : value) {
        const ushort u = c.unicode();
        if (u < 0x20 && u != '\t' && u != '\n' && u != '\r')
            continue; // not representable in XML 1.0
        switch (u) {
        case '&': out += QStringLiteral("&amp;"); break;
        case '<': out += QStringLiteral("&lt;"); break;
        case '>': out += QStringLiteral("&gt;"); break;
        case '"': out += QStringLiteral("&quot;"); break;
        case '\'': out += QStringLiteral("&apos;"); break;
        default: out += c;
        }
    }
    return out;
}

QString textCell(const QString &address, const QString &value, int style)
{
    return QStringLiteral(R"(<c r="%1" s="%2" t="inlineStr"><is><t xml:space="preserve">%3</t></is></c>)")
        .arg(address)
        .arg(style)
        .arg(xmlEscape(value));
}

QString numberCell(const QString &address, double value, int style)
{
    return QStringLiteral(R"(<c r="%1" s="%2"><v>%3</v></c>)")
        .arg(address)
        .arg(style)
        .arg(QLocale::c().toString(value, 'g', QLocale::FloatingPointShortest));
}

QString dateCell(const QString &address, const QString &stamp, int style)
{
    const QDateTime t = parseTimestamp(stamp);
    if (!t.isValid())
        return textCell(address, QString(), 5);
    const double serial = double(QDate(1899, 12, 30).daysTo(t.date()))
        + (t.time().hour() * 60 + t.time().minute()) / 1440.0;
    return numberCell(address, serial, style);
}

QString valueCell(const QString &address, const QString &value, int style)
{
    bool ok = false;
    const int n = value.toInt(&ok);
    Q_UNUSED(style);
    return ok ? numberCell(address, n, 6) : textCell(address, value, 5);
}

const char *kStyles = R"XML(<?xml version="1.0" encoding="UTF-8"?><styleSheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main"><numFmts count="1"><numFmt numFmtId="164" formatCode="yyyy-mm-dd hh:mm"/></numFmts><fonts count="4"><font><sz val="10"/><name val="Microsoft YaHei"/><color rgb="FF243C4B"/></font><font><b/><sz val="16"/><name val="Microsoft YaHei"/><color rgb="FF173746"/></font><font><i/><sz val="10"/><name val="Microsoft YaHei"/><color rgb="FF607985"/></font><font><b/><sz val="10"/><name val="Microsoft YaHei"/><color rgb="FFFFFFFF"/></font></fonts><fills count="3"><fill><patternFill patternType="none"/></fill><fill><patternFill patternType="gray125"/></fill><fill><patternFill patternType="solid"><fgColor rgb="FF087D75"/><bgColor indexed="64"/></patternFill></fill></fills><borders count="2"><border><left/><right/><top/><bottom/><diagonal/></border><border><left/><right/><top/><bottom style="thin"><color rgb="FFDDE8EC"/></bottom><diagonal/></border></borders><cellStyleXfs count="1"><xf numFmtId="0" fontId="0" fillId="0" borderId="0"/></cellStyleXfs><cellXfs count="7"><xf numFmtId="0" fontId="0" fillId="0" borderId="0" xfId="0"/><xf numFmtId="0" fontId="1" fillId="0" borderId="0" xfId="0" applyFont="1" applyAlignment="1"><alignment vertical="center"/></xf><xf numFmtId="0" fontId="2" fillId="0" borderId="0" xfId="0" applyFont="1" applyAlignment="1"><alignment vertical="center"/></xf><xf numFmtId="0" fontId="3" fillId="2" borderId="0" xfId="0" applyFont="1" applyFill="1" applyAlignment="1"><alignment horizontal="center" vertical="center"/></xf><xf numFmtId="164" fontId="0" fillId="0" borderId="1" xfId="0" applyNumberFormat="1" applyAlignment="1"><alignment horizontal="center" vertical="center"/></xf><xf numFmtId="0" fontId="0" fillId="0" borderId="1" xfId="0" applyAlignment="1"><alignment vertical="center" wrapText="1"/></xf><xf numFmtId="0" fontId="0" fillId="0" borderId="1" xfId="0" applyAlignment="1"><alignment horizontal="right" vertical="center"/></xf></cellXfs><cellStyles count="1"><cellStyle name="Normal" xfId="0" builtinId="0"/></cellStyles></styleSheet>)XML";

} // namespace

bool exportXlsx(const QString &path, QList<Activity> rows, const QDateTime &now, QString *error)
{
    rows.erase(std::remove_if(rows.begin(), rows.end(), isIgnored), rows.end());
    sortByRegistrationStart(rows);

    const int last = 4 + qMax(int(rows.size()), 1);
    const QString range = QStringLiteral("A4:K%1").arg(last);
    static const QStringList headers = {
        QStringLiteral("活动名称"), QStringLiteral("活动类型"), QStringLiteral("发起组织"),
        QStringLiteral("报名开始"), QStringLiteral("报名截止"), QStringLiteral("活动地点"),
        QStringLiteral("活动时间"), QStringLiteral("总名额"),   QStringLiteral("剩余名额"),
        QStringLiteral("已报名"),   QStringLiteral("报名状态")};
    static const double widths[] = {34, 14, 30, 19, 19, 30, 27, 11, 11, 11, 14};

    QString sheet;
    sheet += QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?><worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"><sheetViews><sheetView workbookViewId="0" showGridLines="0"><pane ySplit="4" topLeftCell="A5" activePane="bottomLeft" state="frozen"/></sheetView></sheetViews><cols>)");
    for (int i = 0; i < 11; ++i)
        sheet += QStringLiteral(R"(<col min="%1" max="%1" width="%2" customWidth="1"/>)").arg(i + 1).arg(widths[i]);
    sheet += QStringLiteral("</cols><sheetData>");
    sheet += QStringLiteral(R"(<row r="1" ht="30" customHeight="1">)") + textCell(QStringLiteral("A1"), QStringLiteral("未开始活动汇总"), 1)
        + QStringLiteral(R"(</row><row r="2" ht="22" customHeight="1">)")
        + textCell(QStringLiteral("A2"),
                   QStringLiteral("按报名开始时间升序 · 已排除团日、班会和班级活动 · 共 %1 项").arg(rows.size()), 2)
        + QStringLiteral(R"(</row><row r="3" ht="8" customHeight="1"/>)");
    sheet += QStringLiteral(R"(<row r="4" ht="28" customHeight="1">)");
    for (int i = 0; i < headers.size(); ++i)
        sheet += textCell(QStringLiteral("%1%2").arg(QChar('A' + i)).arg(4), headers[i], 3);
    sheet += QStringLiteral("</row>");

    for (int i = 0; i < rows.size(); ++i) {
        const Activity &a = rows[i];
        const int r = 5 + i;
        const RegistrationDates d = registrationDates(a);
        auto addr = [r](char col) { return QStringLiteral("%1%2").arg(QChar(col)).arg(r); };
        sheet += QStringLiteral(R"(<row r="%1" ht="42" customHeight="1">)").arg(r);
        sheet += textCell(addr('A'), a.name, 5);
        sheet += textCell(addr('B'), a.type, 5);
        sheet += textCell(addr('C'), a.organizer, 5);
        sheet += dateCell(addr('D'), d.start, 4);
        sheet += dateCell(addr('E'), d.end, 4);
        sheet += textCell(addr('F'), a.place, 5);
        sheet += textCell(addr('G'), a.activityTime, 5);
        sheet += valueCell(addr('H'), a.capacity, 6);
        sheet += valueCell(addr('I'), a.remaining, 6);
        sheet += valueCell(addr('J'), a.registered, 6);
        sheet += textCell(addr('K'), status(a, now), 5);
        sheet += QStringLiteral("</row>");
    }
    if (rows.isEmpty())
        sheet += QStringLiteral(R"(<row r="5"><c r="A5" t="inlineStr"><is><t>暂无符合条件的活动</t></is></c></row>)");
    sheet += QStringLiteral(R"(</sheetData><mergeCells count="2"><mergeCell ref="A1:K1"/><mergeCell ref="A2:K2"/></mergeCells><autoFilter ref="%1"/><tableParts count="1"><tablePart r:id="rId1"/></tableParts></worksheet>)").arg(range);

    QString table = QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?><table xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" id="1" name="ActivitiesTable" displayName="ActivitiesTable" ref="%1" totalsRowShown="0"><autoFilter ref="%1"/><tableColumns count="11">)").arg(range);
    for (int i = 0; i < headers.size(); ++i)
        table += QStringLiteral(R"(<tableColumn id="%1" name="%2"/>)").arg(i + 1).arg(xmlEscape(headers[i]));
    table += QStringLiteral(R"(</tableColumns><tableStyleInfo name="TableStyleMedium2" showFirstColumn="0" showLastColumn="0" showRowStripes="1" showColumnStripes="0"/></table>)");

    const QString created = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyy-MM-ddTHH:mm:ssZ"));
    const QList<QPair<QString, QString>> parts = {
        {QStringLiteral("[Content_Types].xml"),
         QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?><Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/><Default Extension="xml" ContentType="application/xml"/><Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/><Override PartName="/xl/worksheets/sheet1.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/><Override PartName="/xl/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml"/><Override PartName="/xl/tables/table1.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.table+xml"/><Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/><Override PartName="/docProps/app.xml" ContentType="application/vnd.openxmlformats-officedocument.extended-properties+xml"/></Types>)")},
        {QStringLiteral("_rels/.rels"),
         QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?><Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/><Relationship Id="rId2" Type="http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties" Target="docProps/core.xml"/><Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties" Target="docProps/app.xml"/></Relationships>)")},
        {QStringLiteral("docProps/core.xml"),
         QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?><cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:dcterms="http://purl.org/dc/terms/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"><dc:title>未开始活动汇总</dc:title><dc:creator>广轻活动汇总</dc:creator><dcterms:created xsi:type="dcterms:W3CDTF">%1</dcterms:created></cp:coreProperties>)").arg(created)},
        {QStringLiteral("docProps/app.xml"),
         QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?><Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties"><Application>广轻活动汇总</Application></Properties>)")},
        {QStringLiteral("xl/workbook.xml"),
         QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?><workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"><sheets><sheet name="活动汇总" sheetId="1" r:id="rId1"/></sheets></workbook>)")},
        {QStringLiteral("xl/_rels/workbook.xml.rels"),
         QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?><Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet1.xml"/><Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/></Relationships>)")},
        {QStringLiteral("xl/styles.xml"), QString::fromUtf8(kStyles)},
        {QStringLiteral("xl/worksheets/sheet1.xml"), sheet},
        {QStringLiteral("xl/worksheets/_rels/sheet1.xml.rels"),
         QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?><Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/table" Target="../tables/table1.xml"/></Relationships>)")},
        {QStringLiteral("xl/tables/table1.xml"), table},
    };

    const QString temp = path + QStringLiteral(".tmp");
    QFile::remove(temp);
    {
        QZipWriter zip(temp);
        zip.setCompressionPolicy(QZipWriter::AutoCompress);
        for (const auto &p : parts)
            zip.addFile(p.first, p.second.toUtf8());
        zip.close();
        if (zip.status() != QZipWriter::NoError) {
            if (error)
                *error = QStringLiteral("无法写入 Excel 文件");
            QFile::remove(temp);
            return false;
        }
    }
    QFile::remove(path);
    if (!QFile::rename(temp, path)) {
        if (error)
            *error = QStringLiteral("无法保存到 %1（文件可能正被 Excel 打开）").arg(path);
        QFile::remove(temp);
        return false;
    }
    return true;
}

QByteArray readXlsxPart(const QString &path, const QString &part)
{
    QZipReader zip(path);
    return zip.fileData(part);
}

} // namespace Campus

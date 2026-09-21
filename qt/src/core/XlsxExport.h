#pragma once

#include "Activity.h"

#include <QString>

namespace Campus {

// Writes a styled .xlsx (title, subtitle, frozen header, filter, striped table,
// real date cells). Rows are re-sorted by registration start and ignored
// activities are dropped, exactly like the original exporter.
bool exportXlsx(const QString &path, QList<Activity> rows, const QDateTime &now,
                QString *error = nullptr);

// Test helper: returns the raw text of a part inside the .xlsx package.
QByteArray readXlsxPart(const QString &path, const QString &part);

} // namespace Campus

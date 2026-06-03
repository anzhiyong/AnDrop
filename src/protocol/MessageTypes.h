#pragma once

#include <QString>

namespace MessageTypes {
const QString Announce = QStringLiteral("announce");
const QString Bye = QStringLiteral("bye");
const QString SendRequest = QStringLiteral("send_request");
const QString SendAccept = QStringLiteral("send_accept");
const QString SendReject = QStringLiteral("send_reject");
const QString FileChunk = QStringLiteral("file_chunk");
const QString FileDone = QStringLiteral("file_done");
const QString TransferDone = QStringLiteral("transfer_done");
const QString TransferCancel = QStringLiteral("transfer_cancel");
const QString Error = QStringLiteral("error");
}

#include "TextEdit.h"

TextEdit::TextEdit(QWidget* parent)
	: QPlainTextEdit(parent)
{
    setMouseTracking(true);
}


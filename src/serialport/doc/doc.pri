OTHER_FILES += $$PWD/serialport.qdocconf

docs_target.target = docs
docs_target.commands = qdoc3 $$PWD/serialport.qdocconf

QMAKE_EXTRA_TARGETS = docs_target
QMAKE_CLEAN += "-r $$PWD/html"

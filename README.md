Metta Grabber prototype
=======================

Dependencies
* Qt4 at http://qt-project.org/
* libqxt at http://libqxt.org/
* boost at http://boost.org/
* cmake at http://cmake.org/

* sst from http://pdos.csail.mit.edu/uia/sst/           (included in this repository)
* opus at http://opus-codec.org/                        (included in this repository)
* rtaudio at http://www.music.mcgill.ca/~gary/rtaudio/  (included in this repository)

First prototype requirements
* должна быть возможность добавить устройство в "кластер" своих устройств, разрешая ему таким образом поиск по всему кластеру и возможность модифицировать данные в этом кластере.
* должна быть возможность удалить устройство из кластера, при этом доступа к этому устройству может уже и не быть. Например, потерянный телефон.
* устройства, внесенные в кластер стараются всегда знать статус других устройств в кластере и приоритетно с ними синхронизироваться.
* объект синхронизации - некий блоб и связанные с ним метаданные. структура блоба никак не определена, сохраняются и синхронизируются только изменения. структура метаданных - стандартный key-value storage.
* поиск между устройствами ведется по индексу метаданных, которые устройства стараются синхронизировать приоритетно относительно других объектов.
* если найденные метаданные указывают на блоб, отсутствующий в локальном хранилище, система старается провести поиск этого блоба на других устройствах и синхронизировать его.
* в первой версии ареал хранения можно ограничить только устройствами внутри кластера.

* UIP-like routing (EID's + extended DHT based routing through EID space)

GUI features
* drag-n-drop data to basket
* metadata assignments - how?
* how to perform search? an ESC-interface sounds intriguing

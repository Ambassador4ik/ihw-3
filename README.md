# Мудрое ИДЗ-3
### Игнатенко Дмитрий Александрович БПИ-226 | Вариант 11 

### Задание
Военная задача. Анчуария и Тарантерия — два крохотных латиноамериканских государства, затерянных в южных Андах. Диктатор Анчуарии, дон Федерико, объявил войну диктатору Тарантерии, дону Эрнандо. У обоих диктаторов очень мало солдат, но
очень много снарядов для минометов, привезенных с последней
американской гуманитарной помощью. Поэтому армии обеих сторон просто обстреливают наугад территорию противника, надеясь
поразить что-нибудь ценное. Стрельба ведется по очереди до тех
пор, пока либо не будут уничтожены все К цели (каждая стоимостью Ci), либо стоимость N потраченных снарядов не превысит суммарную стоимость всего того, что ими можно уничтожить.
Размер каждого государства X × Y клеток. Стрельба ведется асинхронно и параллельно (а не последовательно) через случайное время, задаваемое для обеих государств посредством одинаковых алгоритмов генерации случайных чисел. То есть, числа изменяются в
одном и том же диапазоне, но на каждом шаге стрельбы они могут
отличаться для разных государств.
Создать клиент–серверное приложение, моделирующее военные действия.
Каждая страна — отдельный клиент. Сервер отвечает за прием координат от клиентов и передает эти координаты другим
клиентам. Он также получает информацию от клиентов об их
уничтожении. Расположение минометов порождается каждым
клиентом случайно.
Обратить особое внимание на отображение состояния государств в процессе моделирования.

### ???

Отчеты по каждому пункту находятся в соответствующих папках. Там только информация в дополнение к этому readme.

Моя система:
```sh
gcc --version

Apple clang version 15.0.0 (clang-1500.3.9.4)
Target: arm64-apple-darwin23.4.0
Thread model: posix
InstalledDir: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin
```
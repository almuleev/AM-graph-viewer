# Работа с проектом

- Сначала прочитай [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).
- Не сканируй весь репозиторий без необходимости. Выбери область по карте,
  сначала используй `rg` по именам/символам, затем открывай релевантные файлы или участки.
- Не перечитывай неизменённые большие файлы без новой причины. Проверяй diff
  и нужные объявления в `.hpp`; архивы и результаты сборки обычно не нужны.
- Сохраняй посторонние изменения пользователя; правки делай через `apply_patch`.
- Выполняй минимально необходимые проверки и тесты для изменённой области.
  Не повторяй успешные проверки без новых изменений или оснований.
- Перед завершением задачи с изменениями выполни полную сборку CLI и GUI
  командами из ARCHITECTURE.md (Windows PowerShell `powershell.exe` или GNU Make).
  Не обходи ограничения задачи только на чтение. При невозможности сборки
  сообщи конкретную причину и выполненные проверки.
- При изменении границ модулей кратко обновляй карту архитектуры.

## Быстрая карта

- CLI: `main.cpp`; GUI: `gui_main`, `gui_window`, `gui_commands`, `gui_input`.
- Данные: `lvm_parser`, `data_io`; загрузка GUI: `gui_loading`, `gui_loading_drop`.
- FFT: `analysis`, `fft`, `sampling.hpp`, `spectrum_worker`, `gui_spectrum`.
- Обработка: `filter_engine`, `formula_engine`, `gui_processing`.
- График: `gui_render`, `gui_render_data`, `gui_time_axis`, `minmax_index.hpp`.
- Экспорт: `gui_export_metadata`, `export_helpers`; настройки: `gui_settings`,
  `gui_settings_hotkeys`; состояние/история: `gui_state`, `gui_state_history`.
- Тесты: `tests/run_tests.cpp`, `tests/gui_regression.cpp`; история: `CHANGELOG.md`.
- `docs/archive/` — необязательный локальный архив; не добавляй его в Git.

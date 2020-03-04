cmake_minimum_required(VERSION 3.10)

project(charactersheet DESCRIPTION "Shared content to manage charactersheet in rolisteam/rcse")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(QT_REQUIRED_VERSION "5.12.0")
find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED COMPONENTS Core Gui Svg Widgets Quick QuickWidgets WebEngine)

set(sheet_headers ${CMAKE_CURRENT_SOURCE_DIR}/charactersheetmodel.h
              ${CMAKE_CURRENT_SOURCE_DIR}/charactersheet.h
              ${CMAKE_CURRENT_SOURCE_DIR}/charactersheetitem.h
              ${CMAKE_CURRENT_SOURCE_DIR}/field.h
              ${CMAKE_CURRENT_SOURCE_DIR}/tablefield.h
              ${CMAKE_CURRENT_SOURCE_DIR}/csitem.h
              ${CMAKE_CURRENT_SOURCE_DIR}/section.h
              ${CMAKE_CURRENT_SOURCE_DIR}/imagemodel.h
              ${CMAKE_CURRENT_SOURCE_DIR}/sheetwidget.h
              ${CMAKE_CURRENT_SOURCE_DIR}/rolisteamimageprovider.h
              ${CMAKE_CURRENT_SOURCE_DIR}/formula/formulamanager.h
              ${CMAKE_CURRENT_SOURCE_DIR}/formula/parsingtoolformula.h
              ${CMAKE_CURRENT_SOURCE_DIR}/formula/nodes/formulanode.h
              ${CMAKE_CURRENT_SOURCE_DIR}/formula/nodes/startnode.h
              ${CMAKE_CURRENT_SOURCE_DIR}/formula/nodes/operator.h
              ${CMAKE_CURRENT_SOURCE_DIR}/formula/nodes/valuefnode.h
              ${CMAKE_CURRENT_SOURCE_DIR}/formula/nodes/scalaroperatorfnode.h
            ${CMAKE_CURRENT_SOURCE_DIR}/formula/nodes/parenthesesfnode.h)

set(sheet_src
    ${CMAKE_CURRENT_SOURCE_DIR}/charactersheetmodel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/charactersheet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/charactersheetitem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/section.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imagemodel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/field.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tablefield.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/csitem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sheetwidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rolisteamimageprovider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/formula/formulamanager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/formula/parsingtoolformula.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/formula/nodes/formulanode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/formula/nodes/startnode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/formula/nodes/operator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/formula/nodes/valuefnode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/formula/nodes/scalaroperatorfnode.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/formula/nodes/parenthesesfnode.cpp)

set(sheet_qml ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/CheckBoxField.qml
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/DiceButton.qml
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/ImageField.qml
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/main.qml
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/qmldir
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/SelectField.qml
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/Table.qml
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/TextAreaFieldv1.qml
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/TextFieldFieldv1.qml
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/TextInputFieldv1.qml
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/CheckBoxFieldv1.qml
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/DiceButtonv1.qml
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/ImageFieldv1.qml
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/PageButton.qml
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/RLabel.qml
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/SelectFieldv1.qml
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/TextAreaField.qml
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/TextFieldField.qml
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/TextInputField.qml
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/Rolisteam/WebView.qml)

add_library(charactersheet SHARED ${sheet_src} ${sheet_header} ${sheet_qml})

if(${CMAKE_PROJECT_NAME} STREQUAL "rcse")
message("PROJECT NAME = ${CMAKE_PROJECT_NAME}")
target_compile_definitions(charactersheet PRIVATE WITH_PDF=1 RCSE=1)
endif()


target_include_directories(charactersheet PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/formula ${CMAKE_CURRENT_SOURCE_DIR}/formula/nodes ${CMAKE_CURRENT_SOURCE_DIR}/../)
target_link_libraries(charactersheet PUBLIC Qt5::Core Qt5::Gui Qt5::Svg Qt5::Quick Qt5::Widgets Qt5::WebEngine Qt5::QuickWidgets)

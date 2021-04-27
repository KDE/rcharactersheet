/***************************************************************************
 *	 Copyright (C) 2009 by Renaud Guezennec                                *
 *   https://rolisteam.org/contact                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <QAction>
#include <QContextMenuEvent>
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QPrintDialog>
#include <QPrinter>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQuickItem>
#include <QToolButton>

#include "charactersheet/charactersheet.h"
#include "charactersheetwindow.h"
#include "data/character.h"
#include "data/player.h"
#include "preferences/preferencesmanager.h"
#include "sheetwidget.h"

QString parentId(Character* child)
{
    QString id;
    if(!child)
        return id;

    auto parent= child->parentPerson();

    if(!parent)
        return id;

    id= parent->getUuid();

    return id;
}

int findSharingInfoIndex(const CharacterSheet* sheet, const std::vector<SharingInfo>& infos)
{
    auto it= std::find_if(infos.begin(), infos.end(), [sheet](const SharingInfo& info) { return info.sheet == sheet; });

    if(it == infos.end())
        return -1;

    return std::distance(infos.begin(), it);
}

SharingInfo findSharingInfoFromSheet(const CharacterSheet* sheet, const std::vector<SharingInfo>& infos)
{
    auto it= std::find_if(infos.begin(), infos.end(), [sheet](const SharingInfo& info) { return info.sheet == sheet; });

    if(it == infos.end())
        return {};

    return (*it);
}

CharacterSheetWindow::CharacterSheetWindow(QWidget* parent)
    : MediaContainer(nullptr, MediaContainer::ContainerType::VMapContainer, parent)
{
    m_imageModel.reset(new ImageModel());

    setObjectName("CharacterSheetViewer");
    connect(&m_model, &CharacterSheetModel::characterSheetHasBeenAdded, this, [this](CharacterSheet* sheet) {
        auto idx= findSharingInfoIndex(sheet, m_sheetToCharacter);
        if(idx < 0)
            return;
        auto info= m_sheetToCharacter[idx];
        auto tab= addTabWithSheetView(info.sheet, info.character);
        m_sheetToCharacter[idx].tabs.push_back(tab);
    });

    setWindowIcon(QIcon(":/resources/icons/treeview.png"));
    m_addSection= new QAction(tr("Add Section"), this);
    m_addLine= new QAction(tr("Add Field"), this);
    m_addCharacterSheet= new QAction(tr("Add CharacterSheet"), this);
    m_copyTab= new QAction(tr("Copy Tab"), this);
    connect(m_copyTab, SIGNAL(triggered(bool)), this, SLOT(copyTab()));
    m_stopSharingTabAct= new QAction(tr("Stop Sharing"), this);

    m_readOnlyAct= new QAction(tr("Read Only"), this);
    m_readOnlyAct->setCheckable(true);
    connect(m_readOnlyAct, SIGNAL(triggered(bool)), this, SLOT(setReadOnlyOnSelection()));

    m_loadQml= new QAction(tr("Load CharacterSheet View File"), this);

    m_detachTab= new QAction(tr("Detach Tabs"), this);
    m_printAct= new QAction(tr("Print Page"), this);
    m_view.setModel(&m_model);

    resize(m_preferences->value("charactersheetwindows/width", 400).toInt(),
           m_preferences->value("charactersheetwindows/height", 600).toInt());
    m_view.setAlternatingRowColors(true);
    m_view.setSelectionBehavior(QAbstractItemView::SelectItems);
    m_view.setSelectionMode(QAbstractItemView::ExtendedSelection);
    updateTitle();
    m_tabs= new QTabWidget(this);
    m_tabs->addTab(&m_view, tr("Data"));
    setWidget(m_tabs);

    m_view.setContextMenuPolicy(Qt::CustomContextMenu);
    connect(&m_view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayCustomMenu(QPoint)));

    connect(m_addLine, SIGNAL(triggered()), this, SLOT(addLine()));
    connect(m_addSection, SIGNAL(triggered()), this, SLOT(addSection()));
    connect(m_addCharacterSheet, SIGNAL(triggered()), this, SLOT(addCharacterSheet()));
    connect(m_loadQml, SIGNAL(triggered(bool)), this, SLOT(openQML()));
    connect(m_printAct, &QAction::triggered, this, &CharacterSheetWindow::exportPDF);
    connect(m_detachTab, SIGNAL(triggered(bool)), this, SLOT(detachTab()));
    connect(m_stopSharingTabAct, &QAction::triggered, this,
            [this]() { stopSharing(dynamic_cast<SheetWidget*>(m_tabs->currentWidget())); });

    // m_imgProvider= new RolisteamImageProvider(m_imageModel.get());

    auto button= new QToolButton(this); // tr("Actions")
    auto act= new QAction(QIcon("://resources/icons/list.svg"), tr("Actions"), this);
    button->setDefaultAction(act);
    m_tabs->setCornerWidget(button);
    connect(button, &QPushButton::clicked, this,
            [button, this]() { contextMenuForTabs(QPoint(button->pos().x(), 0)); });
}
CharacterSheetWindow::~CharacterSheetWindow() {}
void CharacterSheetWindow::addLine()
{
    m_model.addLine(m_view.currentIndex());
}
void CharacterSheetWindow::setReadOnlyOnSelection()
{
    QList<QModelIndex> list= m_view.selectionModel()->selectedIndexes();
    bool allTheSame= true;
    int i= 0;
    bool firstStatus= true;
    QList<CharacterSheetItem*> listItem;
    for(auto item : list)
    {
        if(0 == item.column())
        {
            CharacterSheetItem* csitem= static_cast<CharacterSheetItem*>(item.internalPointer());
            if(nullptr != csitem)
            {
                listItem.append(csitem);
            }
        }
        else
        {
            CharacterSheet* sheet= m_model.getCharacterSheet(item.column() - 1);
            if(nullptr != sheet)
            {
                CharacterSheetItem* csitem= static_cast<CharacterSheetItem*>(item.internalPointer());
                if(nullptr != csitem)
                {
                    listItem.append(sheet->getFieldFromKey(csitem->getId()));
                }
            }
        }
    }
    for(CharacterSheetItem* csitem : listItem)
    {
        if(nullptr != csitem)
        {
            if(i == 0)
            {
                firstStatus= csitem->isReadOnly();
            }
            if(firstStatus != csitem->isReadOnly())
            {
                allTheSame= false;
            }
        }
        ++i;
    }
    bool valueToSet;
    if(allTheSame)
    {
        valueToSet= !firstStatus;
    }
    else
    {
        valueToSet= true;
    }
    for(CharacterSheetItem* csitem : listItem)
    {
        if(nullptr != csitem)
        {
            csitem->setReadOnly(valueToSet);
        }
    }
}

void CharacterSheetWindow::updateTitle()
{
    setWindowTitle(tr("%1 - (Character Sheet Viewer)").arg(getUriName()));
}

void CharacterSheetWindow::displayCustomMenu(const QPoint& pos)
{
    QMenu menu(this);

    QModelIndex index= m_view.indexAt(pos);
    bool isReadOnly= false;
    if(index.column() > 0)
    {
        m_currentCharacterSheet= m_model.getCharacterSheet(index.column() - 1);
        QMenu* affect= menu.addMenu(m_shareTo);
        addSharingMenu(affect);

        CharacterSheetItem* childItem= static_cast<CharacterSheetItem*>(index.internalPointer());
        if(nullptr != childItem)
        {
            isReadOnly= childItem->isReadOnly();
        }
    }
    menu.addSeparator();
    menu.addAction(m_readOnlyAct);
    m_readOnlyAct->setChecked(isReadOnly);
    /*menu.addSeparator();
    menu.addAction(m_addLine);
    menu.addAction(m_addSection);
    menu.addAction(m_addCharacterSheet);
    menu.addSeparator();
    menu.addAction(m_loadQml);*/

    addActionToMenu(menu);
    menu.exec(QCursor::pos());
}
void CharacterSheetWindow::addSharingMenu(QMenu* share)
{
    /*for(auto& character : PlayerModel::instance()->getCharacterList())
    {
        QAction* action= share->addAction(QPixmap::fromImage(character->getAvatar()), character->name());
        action->setData(character->getUuid());
	connect(action, &QAction::triggered, this, &CharacterSheetWindow::affectSheetToCharacter);
    }*/
}

void CharacterSheetWindow::contextMenuForTabs(const QPoint pos)
{
    QMenu menu(this);

    QWidget* wid= m_tabs->currentWidget();
    SheetWidget* quickWid= dynamic_cast<SheetWidget*>(wid);
    if(nullptr == quickWid)
        return;
    m_currentCharacterSheet= quickWid->sheet();

    if(nullptr != m_currentCharacterSheet)
    {
        menu.addAction(m_detachTab);
        QMenu* share= menu.addMenu(m_shareTo);

        menu.addAction(m_copyTab);
        menu.addAction(m_stopSharingTabAct);
        menu.addSeparator();
        addSharingMenu(share);
        addActionToMenu(menu);
        menu.addAction(m_printAct);

        menu.exec(quickWid->mapToGlobal(pos));
    }
}
bool CharacterSheetWindow::eventFilter(QObject* object, QEvent* event)
{
    if(event->type() == QEvent::Hide)
    {
        SheetWidget* wid= dynamic_cast<SheetWidget*>(object);
        if(nullptr != wid)
        {
            if(-1 == m_tabs->indexOf(wid))
            {
                wid->removeEventFilter(this);
                m_tabs->addTab(wid, wid->windowTitle());
            }
        }
        return MediaContainer::eventFilter(object, event);
    }
    /*else if(event->type() == QEvent::MouseButtonPress)
    {
        qDebug() << "mouse Press";
    }
    else if (event->type() == QEvent::ContextMenu)
    {
       // QQuickItem* wid = dynamic_cast<QQuickItem*>(object);
        auto menuEvent = dynamic_cast<QContextMenuEvent*>(event);
        if( nullptr != menuEvent)//nullptr != wid &&
        {
            contextMenuForTabs(menuEvent->pos());
        }
        qDebug() << "Context Menu";
        return false;
    }*/
    return MediaContainer::eventFilter(object, event);
}

void CharacterSheetWindow::stopSharing(SheetWidget* wid)
{
/*
    if(nullptr == wid)
        return;

    CharacterSheet* sheet= wid->sheet();
    auto idx= findSharingInfoIndex(sheet, m_sheetToCharacter);
    auto character= m_sheetToCharacter[idx].character;

    m_tabs->removeTab(m_tabs->indexOf(wid));

    if(idx < 0)
        return;

    m_sheetToCharacter.erase(m_sheetToCharacter.begin() + idx);

    if(!character)
        return;

    auto parent= character->parentPerson();
    auto id= parent->getUuid();
    NetworkMessageWriter msg(NetMsg::CharacterCategory, NetMsg::closeCharacterSheet);
    QStringList recipiants;
    recipiants << id;
    msg.setRecipientList(recipiants, NetworkMessage::OneOrMany);
    msg.string8(m_mediaId);
    msg.string8(sheet->uuid());
    msg.sendToServer();*/

    SheetWidget* wid= dynamic_cast<SheetWidget*>(m_tabs->currentWidget());
    if(nullptr != wid)
    {
        CharacterSheet* sheet= wid->sheet();
        if(m_sheetToPerson.contains(sheet))
        {
            Player* currentPlayer= m_sheetToPerson.value(sheet);
            if(nullptr == currentPlayer)
                return;

            NetworkMessageWriter msg(NetMsg::CharacterCategory, NetMsg::closeCharacterSheet);
            QStringList recipiants;
            recipiants << currentPlayer->getUuid();
            msg.setRecipientList(recipiants, NetworkMessage::OneOrMany);
            msg.string8(m_mediaId);
            msg.string8(sheet->getUuid());
            /*PlayerModel* list= PlayerModel::instance();
            if(list->hasPlayer(currentPlayer))
            {
                msg.sendToServer();
            }*/
            m_sheetToPerson.remove(sheet);
        }
    }
}

void CharacterSheetWindow::detachTab()
{
    QWidget* wid= m_tabs->currentWidget();
    QString title= m_tabs->tabBar()->tabText(m_tabs->currentIndex());
    m_tabs->removeTab(m_tabs->currentIndex());
    wid->installEventFilter(this);
    emit addWidgetToMdiArea(wid, title);
}
void CharacterSheetWindow::copyTab()
{
    SheetWidget* wid= dynamic_cast<SheetWidget*>(m_tabs->currentWidget());
    if(!wid)
        return;

    auto infoIt
        = std::find_if(std::begin(m_sheetToCharacter), std::end(m_sheetToCharacter), [wid](const SharingInfo& info) {
              return info.tabs.end() != std::find(info.tabs.begin(), info.tabs.end(), wid);
          });

    if(infoIt == m_sheetToCharacter.end())
        return;

    if(!infoIt->sheet || !infoIt->character)
        return;

    auto tab= addTabWithSheetView(infoIt->sheet, infoIt->character);
    infoIt->tabs.push_back(tab);
}

void CharacterSheetWindow::reshareFromDisconnectedList(Character* character)
{
    if(!character)
        return;
    auto key= character->name();

    for(auto info : m_disconnectedCharacter)
    {
        if(info.characterName != key)
            continue;

        auto it= std::find_if(m_sheetToCharacter.begin(), m_sheetToCharacter.end(), [info](const SharingInfo& sharing) {
            return info.characterName == sharing.characterName;
        });

        if(it == m_sheetToCharacter.end())
            affectSheetToCharacter(character, info.sheet);
    }
}

void CharacterSheetWindow::removeAllDisableTab()
{
    std::vector<CharacterSheet*> widgets;

    std::vector<SharingInfo> newDisconnected;
    std::copy_if(m_sheetToCharacter.begin(), m_sheetToCharacter.end(), std::back_inserter(newDisconnected),
                 [](const SharingInfo& info) { return info.character.isNull(); });

    for(auto sharing : newDisconnected)
    {
        auto it= std::find_if(
            m_disconnectedCharacter.begin(), m_disconnectedCharacter.end(), [sharing](const SharingInfo& info) {
                return info.playerName == sharing.playerName && info.characterName == sharing.characterName;
            });
        if(it == m_disconnectedCharacter.end())
            m_disconnectedCharacter.push_back(sharing);
    }

    std::for_each(m_sheetToCharacter.begin(), m_sheetToCharacter.end(), [this](const SharingInfo& info) {
        if(info.character)
            return;
        for(auto w : info.tabs)
            stopSharing(w);
    });

    m_sheetToCharacter.erase(std::remove_if(m_sheetToCharacter.begin(), m_sheetToCharacter.end(),
                                            [](const SharingInfo& info) { return !info.character; }),
                             m_sheetToCharacter.end());
}

void CharacterSheetWindow::shareToCharacter()
{
    QAction* action= qobject_cast<QAction*>(sender());
    QString key= action->data().toString();
    Character* character= PlayersList::instance()->getCharacter(key);
    affectSheetToCharacter(character, m_currentCharacterSheet);
}
/*Character* character= PlayerModel::instance()->getCharacter(key);
    if(nullptr != character)
    {
        CharacterSheet* sheet= m_currentCharacterSheet;
        if(sheet == nullptr)
            return;

void CharacterSheetWindow::affectSheetToCharacter(Character* character, CharacterSheet* sheet)
{
    Player* parent= character->getParentPlayer();

    if(sheet == nullptr || parent == nullptr || character == nullptr)
        return;

    SheetWidget* quickWid= nullptr;
    for(int i= 0, total= m_tabs->count(); i < total; ++i)
    {
        auto wid= dynamic_cast<SheetWidget*>(m_tabs->widget(i));
        if(wid != nullptr && sheet == wid->sheet())
	//if((nullptr != parent) && (nullptr != localItem) && (localItem->isGM()))       
        {
            quickWid= wid;
        }
    }

    checkAlreadyShare(sheet);
    character->setSheet(sheet);
    auto tab= addTabWithSheetView(sheet, character);

    if(nullptr == tab)
        return;

    m_sheetToCharacter.push_back({sheet, character, {tab}, parent->name(), character->name()});
    sheet->setName(character->name());
    m_tabs->setTabText(m_tabs->indexOf(quickWid), sheet->name());

    Player* localItem= PlayersList::instance()->getLocalPlayer();
    if((nullptr != parent) && (nullptr != localItem) && (localItem->isGM()))
    }
}*/

void CharacterSheetWindow::checkAlreadyShare(CharacterSheet* sheet)
{
    if(m_sheetToPerson.contains(sheet))
    {
        Player* person= character->getParentPlayer();
        if(nullptr != person)
        {

            NetworkMessageWriter msg(NetMsg::CharacterCategory, NetMsg::closeCharacterSheet);
            QStringList recipiants;
            recipiants << olderParent->getUuid();
            msg.setRecipientList(recipiants, NetworkMessage::OneOrMany);
            msg.string8(m_mediaId);
            msg.string8(sheet->getUuid());
            /*PlayerModel* list= PlayerModel::instance();
            if(list->hasPlayer(olderParent))
            {
                msg.sendToServer();
            }*/
        }
    }
}

void CharacterSheetWindow::checkAlreadyShare(CharacterSheet* sheet)
{

    auto idx= findSharingInfoIndex(sheet, m_sheetToCharacter);
    if(idx < 0)
        return;

    auto oldCharacter= m_sheetToCharacter[idx].character;

    auto id= parentId(oldCharacter);
    m_sheetToCharacter.erase(m_sheetToCharacter.begin() + idx);

    if(id.isEmpty())
        return;

    NetworkMessageWriter msg(NetMsg::CharacterCategory, NetMsg::closeCharacterSheet);
    QStringList recipiants;
    recipiants << id;
    msg.setRecipientList(recipiants, NetworkMessage::OneOrMany);
    msg.string8(m_mediaId);
    msg.string8(sheet->uuid());
    msg.sendToServer();
}
bool CharacterSheetWindow::hasCharacterSheet(QString id)
{
    return (m_model.getCharacterSheetById(id) != nullptr);
}

void CharacterSheetWindow::addSection()
{
    m_model.addSection();
}
void CharacterSheetWindow::addCharacterSheet()
{
    m_errorList.clear();
    m_model.addCharacterSheet();
    displayError(m_errorList);
}
SheetWidget* CharacterSheetWindow::addTabWithSheetView(CharacterSheet* chSheet, Character* character)
{
    if(chSheet == nullptr || character == nullptr)
        return nullptr;

    if(m_qmlData.isEmpty())
    {
        openQML();
    }
    auto qmlView= new SheetWidget(this);
    connect(qmlView, &SheetWidget::customMenuRequested, this, &CharacterSheetWindow::contextMenuForTabs);

    auto imageProvider= new RolisteamImageProvider(m_imageModel.get());

    auto engineQml= qmlView->engine();

    engineQml->addImageProvider(QLatin1String("rcs"), imageProvider);
    engineQml->addImportPath(QStringLiteral("qrc:/charactersheet/qml"));

    qmlView->engine()->rootContext()->setContextProperty("_character", character);

    for(int i= 0; i < chSheet->getFieldCount(); ++i)
    {
        CharacterSheetItem* field= chSheet->getFieldAt(i);
        if(nullptr != field)
        {
            qmlView->engine()->rootContext()->setContextProperty(field->getId(), field);
        }
    }

    QTemporaryFile fileTemp;

    if(fileTemp.open()) // QIODevice::WriteOnly
    {
        fileTemp.write(m_qmlData.toUtf8());
        fileTemp.close();
    }

    qmlView->setSource(QUrl::fromLocalFile(fileTemp.fileName()));
    qmlView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    readErrorFromQML(qmlView->errors());
    m_errorList.append(qmlView->errors());

    QObject* root= qmlView->rootObject();

    // CONNECTION TO SIGNAL FROM QML CHARACTERSHEET
    connect(root, SIGNAL(showText(QString)), this, SIGNAL(showText(QString)));
    connect(root, SIGNAL(rollDiceCmd(QString, bool)), this, SLOT(rollDice(QString, bool)));
    connect(root, SIGNAL(rollDiceCmd(QString)), this, SLOT(rollDice(QString)));

    qmlView->setSheet(chSheet);
    int id= m_tabs->addTab(qmlView, chSheet->getTitle());
    if(!m_localIsGM)
    {
        m_tabs->setCurrentIndex(id);
    }

    return qmlView;
}
void CharacterSheetWindow::readErrorFromQML(QList<QQmlError> list)
{
    for(auto& error : list)
    {
        emit errorOccurs(error.toString());
    }
}

void CharacterSheetWindow::rollDice(QString str, bool alias)
{
    QObject* obj= sender();
    QString label;
    for(int i= 0, total= m_tabs->count(); i < total; ++i)
    {
        SheetWidget* qmlView= dynamic_cast<SheetWidget*>(m_tabs->widget(i));

        if(nullptr == qmlView)
            continue;

        QObject* root= qmlView->rootObject();
        if(root == obj)
        {
            label= m_tabs->tabText(m_tabs->indexOf(qmlView));
        }
    }
    emit rollDiceCmd(str, label, alias);
}

void CharacterSheetWindow::updateFieldFrom(CharacterSheet* sheet, CharacterSheetItem* item, const QString& parentPath)
{
    if(nullptr == sheet)
        return;

    QString id;

    id= parentId(findSharingInfoFromSheet(sheet, m_sheetToCharacter).character);

    if(!m_localIsGM)
    {
        auto person= PlayersList::instance()->getGM();
        id= person->getUuid();
        Player* person= nullptr;
        if(m_localIsGM)
        {
            person= m_sheetToPerson.value(sheet);
        }
        else
        {
            // person= PlayerModel::instance()->getGM();
        }
        if(nullptr != person)
        {
            NetworkMessageWriter msg(NetMsg::CharacterCategory, NetMsg::updateFieldCharacterSheet);
            QStringList recipiants;
            recipiants << person->getUuid();
            msg.setRecipientList(recipiants, NetworkMessage::OneOrMany);
            msg.string8(m_mediaId);
            msg.string8(sheet->getUuid());
            msg.string32(parentPath);
            QJsonObject object;
            item->saveDataItem(object);
            QJsonDocument doc;
            doc.setObject(object);
            msg.byteArray32(doc.toBinaryData());
            msg.sendToServer();
        }
    }

    if(id.isEmpty())
        return;

    NetworkMessageWriter msg(NetMsg::CharacterCategory, NetMsg::updateFieldCharacterSheet);
    QStringList recipiants;
    recipiants << id;
    msg.setRecipientList(recipiants, NetworkMessage::OneOrMany);
    msg.string8(m_mediaId);
    msg.string8(sheet->uuid());
    msg.string32(parentPath);
    QJsonObject object;
    item->saveDataItem(object);
    QJsonDocument doc;
    doc.setObject(object);
    msg.byteArray32(doc.toBinaryData());
    msg.sendToServer();
}
void CharacterSheetWindow::processUpdateFieldMessage(NetworkMessageReader* msg, const QString& idSheet)
{
    CharacterSheet* currentSheet= m_model.getCharacterSheetById(idSheet);

    if(nullptr == currentSheet)
        return;

    auto path= msg->string32();
    QByteArray array= msg->byteArray32();
    if(array.isEmpty())
        return;

    QJsonDocument doc= QJsonDocument::fromBinaryData(array);
    QJsonObject obj= doc.object();
    currentSheet->setFieldData(obj, path);
}
void CharacterSheetWindow::displayError(const QList<QQmlError>& warnings)
{
    QString result= "";
    for(auto& error : warnings)
    {
        result+= error.toString();
    }
    if(!warnings.isEmpty())
    {
        QMessageBox::information(this, tr("QML Errors"), result, QMessageBox::Ok);
    }
}

QJsonDocument CharacterSheetWindow::saveFile(const QString& formerPath)
{

    /*if(nullptr == m_uri)
    {
        qWarning() << "no uri for charactersheet";
        return {};
    }

    QJsonDocument json;
    QJsonObject obj;
    auto path= m_uri->getUri();

    QByteArray data;
    auto pathExist= QFileInfo::exists(path);
    if(path.isEmpty())
    {
        data= m_uri->getData();
    }
    else
    {
        QFile file(pathExist ? path : formerPath);
        if(file.open(QIODevice::ReadOnly))
        {
            data= file.readAll();
            file.close();
        }
    }

    if(data.isEmpty())
    {
        qWarning() << "Charactersheet with empty data";
        return {};
    }

    json= QJsonDocument::fromJson(data);
    obj= json.object();

    // Get datamodel
    obj["data"]= m_data;

    // qml file
    // obj["qml"]=m_qmlData;*/

    // background
    /*QJsonArray images = obj["background"].toArray();
    for(auto key : m_pixmapList->keys())
    {
        QJsonObject obj;
        obj["key"]=key;
        obj["isBg"]=;
        QPixmap pix = m_pixmapList->value(key);
        if(!pix.isNull())
        {
            QByteArray bytes;
            QBuffer buffer(&bytes);
            buffer.open(QIODevice::WriteOnly);
            pix.save(&buffer, "PNG");
            obj["bin"]=QString(buffer.data().toBase64());
            images.append(obj);
        }
    }
    obj["background"]=images;*/
    /* m_model.writeModel(obj, false);
     json.setObject(obj);
     return json;*/
    return {};
}

bool CharacterSheetWindow::readData(QByteArray data)
{
    QJsonDocument json= QJsonDocument::fromJson(data);
    QJsonObject jsonObj= json.object();

    m_data= jsonObj["data"].toObject();

    m_qmlData= jsonObj["qml"].toString();

    QJsonArray images= jsonObj["background"].toArray();
    for(auto jsonpix : images)
    {
        QJsonObject obj= jsonpix.toObject();
        QString str= obj["bin"].toString();
        QString key= obj["key"].toString();
        QString filename= obj["filename"].toString();
        bool isBg= obj["isBg"].toBool();
        QByteArray array= QByteArray::fromBase64(str.toUtf8());
        QPixmap pix;
        pix.loadFromData(array);
        m_imageModel->insertImage(pix, key, filename, isBg);
    }

    const auto fonts= jsonObj["fonts"].toArray();
    for(const auto& obj : fonts)
    {
        const auto font= obj.toObject();
        const auto fontData= QByteArray::fromBase64(font["data"].toString("").toLatin1());
        QFontDatabase::addApplicationFontFromData(fontData);
    }

    m_errorList.clear();
    m_model.readModel(jsonObj, true);
    displayError(m_errorList);

    for(int j= 0; j < m_model.getCharacterSheetCount(); ++j)
    {
        CharacterSheet* sheet= m_model.getCharacterSheet(j);
        if(nullptr != sheet)
        {
            connect(sheet, &CharacterSheet::updateField, this, &CharacterSheetWindow::updateFieldFrom);
        }
    }
    return true;
}

bool CharacterSheetWindow::openFile(const QString& fileUri)
{
    if(!fileUri.isEmpty())
    {
        QFile file(fileUri);
        if(file.open(QIODevice::ReadOnly))
        {
            readData(file.readAll());
        }
        return true;
    }
    return false;
}

void CharacterSheetWindow::openQML()
{
    m_qmlUri= QFileDialog::getOpenFileName(this, tr("Open Character Sheets View"),
                                           m_preferences->value(QString("DataDirectory"), QVariant(".")).toString(),
                                           tr("Character Sheet files (*.qml)"));
    if(!m_qmlUri.isEmpty())
    {
        QFile file(m_qmlUri);
        if(file.open(QIODevice::ReadOnly))
        {
            m_qmlData= QString(file.readAll());
        }
    }
}

void CharacterSheetWindow::closeEvent(QCloseEvent* event)
{
    setVisible(false);
    event->ignore();
}

QString CharacterSheetWindow::getQmlData() const
{
    return m_qmlData;
}

void CharacterSheetWindow::setQmlData(const QString& qmlData)
{
    m_qmlData= qmlData;
}

void CharacterSheetWindow::addCharacterSheet(CharacterSheet* sheet)
{
    if(nullptr != sheet)
    {
        connect(sheet, &CharacterSheet::updateField, this, &CharacterSheetWindow::updateFieldFrom);
        m_model.addCharacterSheet(sheet, false);
    }
}

bool CharacterSheetWindow::readFileFromUri()
{
    /*  if(nullptr != m_uri)
      {
          updateTitle();

          if(m_uri->getCurrentMode() == CleverURI::Internal || !m_uri->exists())
          {
              QByteArray data= m_uri->getData();
              m_uri->setCurrentMode(CleverURI::Internal);
              QJsonDocument doc= QJsonDocument::fromBinaryData(data);
              return readData(doc.toJson());
          }
          else if(m_uri->getCurrentMode() == CleverURI::Linked)
          {
              return openFile(m_uri->getUri());
          }
      }*/
    return false;
}
void CharacterSheetWindow::putDataIntoCleverUri()
{
    QByteArray data;
    QJsonDocument doc= saveFile();
    data= doc.toBinaryData();
    /* if(nullptr != m_uri)
     {
         m_uri->setData(data);
     }*/
}

void CharacterSheetWindow::saveMedia(const QString& formerPath)
{
    /*if((nullptr != m_uri) && (!m_uri->getUri().isEmpty()))
    {
        QString uri= m_uri->getUri();
        if(!uri.isEmpty())
        {
            if(!uri.endsWith(".rcs"))
            {
                uri+= QLatin1String(".rcs");
            }
            QJsonDocument doc= saveFile(formerPath);
            QFile file(uri);
            if(file.open(QIODevice::WriteOnly) && !doc.isEmpty())
            {
                file.write(doc.toJson());
                file.close();
            }
        }
    }*/
}
void CharacterSheetWindow::fillMessage(NetworkMessageWriter* msg, CharacterSheet* sheet, QString idChar)
{
    msg->string8(m_mediaId);
    msg->string8(idChar);
    msg->string8(getUriName());

    if(nullptr != sheet)
    {
        sheet->fill(*msg);
    }
    msg->string32(m_qmlData);
    m_imageModel->fill(*msg);

    m_model.fillRootSection(msg);
}

void CharacterSheetWindow::readMessage(NetworkMessageReader& msg)
{
    CharacterSheet* sheet= new CharacterSheet();

    m_mediaId= msg.string8();
    QString idChar= msg.string8();
    setUriName(msg.string8());

    if(nullptr != sheet)
    {
        sheet->read(msg);
    }
    m_qmlData= msg.string32();
    m_imageModel->read(msg);

    /*Character* character= PlayerModel::instance()->getCharacter(idChar);
    m_sheetToCharacter.insert(sheet, character);

    addCharacterSheet(sheet);
    m_model.readRootSection(&msg);
    if(nullptr != character)
    {
        character->setSheet(sheet);
    }*/
}

void CharacterSheetWindow::exportPDF()
{
    auto qmlView= qobject_cast<SheetWidget*>(m_tabs->currentWidget());

    if(nullptr == qmlView)
        return;

    QObject* root= qmlView->rootObject();
    if(nullptr == root)
        return;

    auto maxPage= QQmlProperty::read(root, "maxPage").toInt();
    auto currentPage= QQmlProperty::read(root, "page").toInt();
    auto sheetW= QQmlProperty::read(root, "width").toReal();
    auto sheetH= QQmlProperty::read(root, "height").toReal();

    QObject* imagebg= root->findChild<QObject*>("imagebg");
    if(nullptr != imagebg)
    {
        sheetW= QQmlProperty::read(imagebg, "width").toReal();
        sheetH= QQmlProperty::read(imagebg, "height").toReal();
    }
    else
    {
        sheetW= width();
        sheetH= height();
    }
    QPrinter printer;
    QPrintDialog dialog(&printer, this);
    if(dialog.exec() == QDialog::Accepted)
    {
        QPainter painter;
        if(painter.begin(&printer))
        {
            for(int i= 0; i <= maxPage; ++i)
            {
                root->setProperty("page", i);
                QTimer timer;
                timer.setSingleShot(true);
                QEventLoop loop;
                connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
                timer.start(m_preferences->value("waitingTimeBetweenPage", 300).toInt());
                loop.exec();

                auto image= qmlView->grabFramebuffer();
                QRectF rect(0, 0, printer.width(), printer.height());
                QRectF source(0.0, 0.0, sheetW, sheetH);
                painter.drawImage(rect, image, source);
                if(i != maxPage)
                    printer.newPage();
            }
            painter.end();
        }
    }
    root->setProperty("page", currentPage);
}

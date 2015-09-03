#include "NetworkItem.h"

NetworkItem::NetworkItem(int id,
                         NodeType itemType,
                         int row, int col,
                         bool selected, bool enabled,
                         int coop_id, bool ava){
    _id = id;
    _itemType = itemType;
    _row = row;
    _col = col;
    _selected = selected;
    _enabled = enabled;
    _cooperant_id = coop_id;
    _available = ava;
    _guiIdx = -1;
}

int NetworkItem::getId(){
    return _id;
}

int NetworkItem::getGuiIdx() const{
    return _guiIdx;
}

void NetworkItem::setGuiIdx(int guiIdx){
    _guiIdx = guiIdx;
}

NodeType NetworkItem::itemType() const{
    return _itemType;
}

int NetworkItem::row() const{
    return _row;
}

int NetworkItem::col() const{
    return _col;
}

bool NetworkItem::selected() const{
    return _selected;
}

void NetworkItem::setSelected(bool selected){
    _selected = selected;
}

bool NetworkItem::enabled() const{
    return _enabled;
}

void NetworkItem::setEnabled(bool enabled){
    _enabled = enabled;
}

int NetworkItem::cooperant_id() const{
    return _cooperant_id;
}

void NetworkItem::setCooperant_id(int cooperant_id){
    _cooperant_id = cooperant_id;
}

bool NetworkItem::available() const{
    return _available;
}

void NetworkItem::setAvailable(bool available){
    _available = available;
}

#ifndef NETWORKITEM_H
#define NETWORKITEM_H

#include<TestbedTypes.h>
#include<vector>
#include<string>

class NetworkItem{

    public:
        explicit NetworkItem(int id, NodeType itemType, int row, int col, bool selected = false,
                             bool enabled = true, int coop_id = -1, bool ava = false);

    int getId();

    void setGuiIdx(int guiIdx);
    int getGuiIdx() const;

    NodeType itemType() const;
    int row() const;
    int col() const;

    bool selected() const;
    void setSelected(bool selected);

    bool enabled() const;
    void setEnabled(bool enabled);

    int cooperant_id() const;
    void setCooperant_id(int cooperant_id);

    bool available() const;
    void setAvailable(bool available);

private:

    NodeType _itemType;
    int _id;
    int _guiIdx;
    int _row;
    int _col;
    bool _selected;
    bool _enabled;
    int _cooperant_id;
    bool _available;

};

#endif // NETWORKITEM_H

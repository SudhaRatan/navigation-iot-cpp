#pragma once
#include "TFT_eSPI.h"
#include <vector>

class List
{
private:
  TFT_eSprite *tft;
  int selectedIndex;
  std::vector<String> items;
  std::vector<const uint16_t*> images;
  
public:

  List(TFT_eSprite &_tft, std::vector<String> &_items, std::vector<const uint16_t*> &_images);
  void draw();
  void setSelectedIndex(int index);
  int getSelectedIndex();
  int length();
};
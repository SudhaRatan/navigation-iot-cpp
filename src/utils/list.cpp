#include "../headers/list.h"

List::List(TFT_eSprite &_tft, std::vector<String> &_items, std::vector<const uint16_t *> &_images)
    : tft(&_tft), selectedIndex(0), items(_items), images(_images) {}
const int width = 240, height = 240;
const int main_height = 18;
const int sec_height = 13;
const int main_length = 13;
const int sec_length = 10;

void List::draw()
{

  tft->createSprite(240, 240);
  tft->fillSprite(TFT_BLACK);
  tft->setTextSize(1);
  int starting_index = selectedIndex;
  for (size_t i = 0; i < items.size(); ++i)
  {
    String text = items[i];
    uint16_t color = (i == selectedIndex) ? TFT_YELLOW : TFT_WHITE;
    if (i == selectedIndex)
    {
      tft->fillRoundRect(5, 103, 230, 35, 15, 0xFFFF);
      tft->setFreeFont(&FreeSansBold12pt7b);
      tft->setTextColor(0x4208);
      tft->drawString(text, width * 0.5 - (text.length() * main_length) * 0.5, height * 0.5 - main_height * 0.5);
      if (i < images.size() && images[i])
      {
        tft->pushImage(208, 110, 20, 20, images[i]);
      }
    }
    else if (i < selectedIndex && selectedIndex - i <= 2)
    {
      tft->setFreeFont(selectedIndex - i == 1 ? &FreeSansBold9pt7b : &FreeSans9pt7b);
      tft->setTextColor(0xFFFF);
      tft->drawString(text, width * 0.5 - (text.length() * sec_length) * 0.5, height * 0.5 - main_height - sec_height * (selectedIndex - i) - (20 * (selectedIndex - i)) - main_height * 0.5);
    }
    else if (i > selectedIndex && i - selectedIndex <= 2)
    {
      tft->setFreeFont(i - selectedIndex == 1 ? &FreeSansBold9pt7b : &FreeSans9pt7b);
      tft->setTextColor(0xFFFF);
      tft->drawString(text, width * 0.5 - (text.length() * sec_length) * 0.5, height * 0.5 + main_height + sec_height * (i - selectedIndex) + (20 * (i - selectedIndex)));
    }
  }
  if(selectedIndex < items.size() - 1) {
    tft->fillTriangle(120, 155, 125, 150, 115, 150, 0xBDF7);
  }
  if(selectedIndex > 0) {
    tft->fillTriangle(120, 85, 125, 90, 115, 90, 0xBDF7);
  }
  tft->pushSprite(0, 0);
}

void List::setSelectedIndex(int index)
{
  if (index >= 0 && index < items.size())
  {
    selectedIndex = index;
    List::draw();
  }
}
int List::getSelectedIndex()
{
  return selectedIndex;
}

int List::length()
{
  return items.size();
}
object fmMain: TfmMain
  Left = 0
  Top = 0
  Caption = 'Coins Collection'
  ClientHeight = 441
  ClientWidth = 784
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  Menu = MainMenu
  OldCreateOrder = False
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 13
  object btnSelfTest: TButton
    Left = 16
    Top = 16
    Width = 105
    Height = 25
    Caption = 'DB self-test'
    TabOrder = 0
    OnClick = btnSelfTestClick
  end
  object MainMenu: TMainMenu
    Left = 200
    Top = 16
    object miSettings: TMenuItem
      Caption = 'Settings'
      object miLanguage: TMenuItem
        Caption = 'Language'
      end
    end
  end
end

object fmNumistaChoose: TfmNumistaChoose
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  Caption = 'Choose Numista type'
  ClientHeight = 340
  ClientWidth = 640
  Color = clBtnFace
  Constraints.MinHeight = 250
  Constraints.MinWidth = 480
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  Position = poOwnerFormCenter
  OnResize = FormResize
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object lblCoin: TLabel
    AlignWithMargins = True
    Left = 8
    Top = 8
    Width = 624
    Height = 13
    Margins.Left = 8
    Margins.Top = 8
    Margins.Right = 8
    Margins.Bottom = 6
    Align = alTop
    ShowAccelChar = False
  end
  object pnlQuery: TPanel
    Left = 0
    Top = 27
    Width = 640
    Height = 32
    Align = alTop
    BevelOuter = bvNone
    TabOrder = 1
    DesignSize = (
      640
      32)
    object lblQuery: TLabel
      Left = 8
      Top = 8
      Width = 80
      Height = 13
      Caption = 'Search &phrase:'
      FocusControl = edQuery
    end
    object edQuery: TEdit
      Left = 112
      Top = 4
      Width = 420
      Height = 21
      Anchors = [akLeft, akTop, akRight]
      TabOrder = 0
      OnEnter = edQueryEnter
      OnExit = edQueryExit
    end
    object btnSearch: TButton
      Left = 540
      Top = 3
      Width = 92
      Height = 23
      Anchors = [akTop, akRight]
      Caption = '&Search'
      TabOrder = 1
      OnClick = btnSearchClick
    end
  end
  object dgTypes: TDrawGrid
    AlignWithMargins = True
    Left = 8
    Top = 59
    Width = 624
    Height = 234
    Margins.Left = 8
    Margins.Top = 0
    Margins.Right = 8
    Margins.Bottom = 0
    Align = alClient
    ColCount = 4
    DefaultRowHeight = 18
    FixedCols = 0
    RowCount = 2
    Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goColSizing, goRowSelect, goDrawFocusSelected, goThumbTracking]
    TabOrder = 0
    OnDblClick = dgTypesDblClick
    OnDrawCell = dgTypesDrawCell
    OnSelectCell = dgTypesSelectCell
  end
  object pnlButtons: TPanel
    Left = 0
    Top = 293
    Width = 640
    Height = 47
    Align = alBottom
    BevelOuter = bvNone
    TabOrder = 2
    DesignSize = (
      640
      47)
    object btnOpen: TButton
      Left = 8
      Top = 12
      Width = 130
      Height = 25
      Caption = '&Open in Numista'
      TabOrder = 0
      OnClick = btnOpenClick
    end
    object btnLink: TButton
      Left = 452
      Top = 12
      Width = 90
      Height = 25
      Anchors = [akTop, akRight]
      Caption = '&Link'
      Default = True
      ModalResult = 1
      TabOrder = 1
    end
    object btnCancel: TButton
      Left = 548
      Top = 12
      Width = 84
      Height = 25
      Anchors = [akTop, akRight]
      Cancel = True
      Caption = 'Cancel'
      ModalResult = 2
      TabOrder = 2
    end
  end
end

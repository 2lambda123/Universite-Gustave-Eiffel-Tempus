from PyQt4.QtCore import *
from PyQt4.QtGui import *
import config

class CriterionChooser( QWidget ):

    criterion_id = { "Distance" : 1, "Duration" : 2, "Price" : 3, "Carbon" : 4, "Calories" : 5, "NumberOfChanges" : 6, "Variability" : 7 }

    def __init__( self, parent, first = False ):
        QWidget.__init__( self )
        self.parent = parent

        self.layout = QHBoxLayout( self )
        self.layout.setMargin( 0 )

        self.label = QLabel( self )
        self.label.setText( "Criterion" )

        self.criterionList = QComboBox( self )        
        n = 0
        for k,v in CriterionChooser.criterion_id.items():
            self.criterionList.insertItem( n, k )
            n += 1

        self.btn = QToolButton( self )
        if first:
            self.btn.setIcon( QIcon( config.DATA_DIR + "/add.png" ) )
            QObject.connect( self.btn, SIGNAL("clicked()"), self.onAdd )
        else:
            self.btn.setIcon( QIcon( config.DATA_DIR + "/remove.png" ) )
            QObject.connect( self.btn, SIGNAL("clicked()"), self.onRemove )

        self.layout.addWidget( self.label )
        self.layout.addWidget( self.criterionList )
        self.layout.addWidget( self.btn )

    def onAdd( self ):
        # daddy is supposed to be a QLayout here
        self.parent.addWidget( CriterionChooser( self.parent ) )

    def onRemove( self ):
        self.parent.removeWidget( self )
        self.close()
        self.parent.update()

    def selected( self ):
        return CriterionChooser.criterion_id[ str(self.criterionList.currentText()) ]

    def set_selection( self, selection_id ):
        # look for this ID
        name = ''
        for k,nid in CriterionChooser.criterion_id.items():
            if nid == selection_id:
               name = k
               break
        # select based on a name
        for i in range(0, self.criterionList.count()):
            t = self.criterionList.itemText( i )
            if t == name:
                self.criterionList.setCurrentIndex( i )
                break
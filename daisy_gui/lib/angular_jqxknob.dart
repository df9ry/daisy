/*
jQWidgets v4.5.4 (2017-June)
Copyright (c) 2011-2017 jQWidgets.
License: http://jqwidgets.com/license/
*/

/// <reference path="jqwidgets.d.ts" />
import "package:angular2/core.dart" show
Component, Input, Output, EventEmitter, ElementRef, OnChanges,
SimpleChanges, ChangeDetectionStrategy;

dynamic JQXLite;

@Component(
    selector: "jqxKnob",
    template: "<div><ng-content></ng-content></div>"
)
class jqxKnobComponent implements OnChanges {
  @Input("allowValueChangeOnClick") dynamic attrAllowValueChangeOnClick;
  @Input("allowValueChangeOnDrag") dynamic attrAllowValueChangeOnDrag;
  @Input(
      "allowValueChangeOnMouseWheel") dynamic attrAllowValueChangeOnMouseWheel;
  @Input("changing") dynamic attrChanging;
  @Input("dragEndAngle") dynamic attrDragEndAngle;
  @Input("dragStartAngle") dynamic attrDragStartAngle;
  @Input("disabled") dynamic attrDisabled;
  @Input("dial") dynamic attrDial;
  @Input("endAngle") dynamic attrEndAngle;
  @Input("labels") dynamic attrLabels;
  @Input("marks") dynamic attrMarks;
  @Input("min") dynamic attrMin;
  @Input("max") dynamic attrMax;
  @Input("progressBar") dynamic attrProgressBar;
  @Input("pointer") dynamic attrPointer;
  @Input("pointerGrabAction") dynamic attrPointerGrabAction;
  @Input("rotation") dynamic attrRotation;
  @Input("startAngle") dynamic attrStartAngle;
  @Input("spinner") dynamic attrSpinner;
  @Input("style") dynamic attrStyle;
  @Input("step") dynamic attrStep;
  @Input("snapToStep") dynamic attrSnapToStep;
  @Input("value") dynamic attrValue;
  @Input("width") dynamic attrWidth;
  @Input("height") dynamic attrHeight;
  @Input("auto-create") bool autoCreate = true;
  List <String> properties = [
    "allowValueChangeOnClick", "allowValueChangeOnDrag",
    "allowValueChangeOnMouseWheel", "changing", "dragEndAngle",
    "dragStartAngle", "disabled", "dial", "endAngle", "height",
    "labels", "marks", "min", "max", "progressBar", "pointer",
    "pointerGrabAction", "rotation", "startAngle", "spinner", "style",
    "step", "snapToStep", "value", "width"
  ];

  dynamic host;
  ElementRef elementRef;
  jqwidgets.jqxKnob widgetObject;

  jqxKnobComponent(ElementRef containerElement) {
    this.elementRef = containerElement;
    JQXLite(window).resize(() {
      this.___updateRect__();
    });
  }

  ngOnInit() {
    if (this.autoCreate) {
      this.createComponent();
    }
  }

  ngOnChanges(SimpleChanges changes) {
    if (this.host) {
      for (var i = 0; i < this.properties.length; i ++) {
        var attrName = "attr" +
            this.properties [ i ].substring(0, 1).toUpperCase() +
            this.properties [ i ].substring(1);
        bool areEqual;
        if (!identical(this [ attrName ], undefined)) {
          if (identical(this[attrName], "object")) {
            if (this [ attrName ] is Array) {
              areEqual = this.arraysEqual(
                  this [ attrName ], this.host.jqxKnob(this.properties [ i ]));
            }
            if (areEqual) {
              return false;
            }
            this.host.jqxKnob(this.properties [ i ], this [ attrName ]);
            continue;
          }
          if (!identical(
              this [ attrName ], this.host.jqxKnob(this.properties [ i ]))) {
            this.host.jqxKnob(this.properties [ i ], this [ attrName ]);
          }
        }
      }
    }
  }

  bool arraysEqual(dynamic attrValue, dynamic hostValue) {
    if (attrValue.length != hostValue.length) {
      return false;
    }
    for (var i = 0; i < attrValue.length; i ++) {
      if (!identical(attrValue [ i ], hostValue [ i ])) {
        return false;
      }
    }
    return true;
  }

  dynamic manageAttributes() {
    var options = {};
    for (var i = 0; i < this.properties.length; i ++) {
      var attrName = "attr" +
          this.properties [ i ].substring(0, 1).toUpperCase() +
          this.properties [ i ].substring(1);
      if (!identical(this [ attrName ], undefined)) {
        options [ this.properties [ i ] ] = this [ attrName ];
      }
    }
    return options;
  }

  void createComponent([ dynamic options ]) {
    if (options) {
      JQXLite.extend(options, this.manageAttributes());
    } else {
      options = this.manageAttributes();
    }
    this.host = JQXLite(this.elementRef.nativeElement.firstChild);
    this.___wireEvents__();
    this.widgetObject =
        jqwidgets.createInstance(this.host, "jqxKnob", options);
    this.___updateRect__();
  }

  void createWidget([ dynamic options ]) {
    this.createComponent(options);
  }

  void ___updateRect__() {
    this.host.css(width: this.attrWidth, height: this.attrHeight);
  }

  void setOptions(dynamic options) {
    this.host.jqxKnob("setOptions", options);
  }

  // jqxKnobComponent properties
  dynamic allowValueChangeOnClick([ bool arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("allowValueChangeOnClick", arg);
    } else {
      return this.host.jqxKnob("allowValueChangeOnClick");
    }
  }

  dynamic allowValueChangeOnDrag([ bool arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("allowValueChangeOnDrag", arg);
    } else {
      return this.host.jqxKnob("allowValueChangeOnDrag");
    }
  }

  dynamic allowValueChangeOnMouseWheel([ bool arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("allowValueChangeOnMouseWheel", arg);
    } else {
      return this.host.jqxKnob("allowValueChangeOnMouseWheel");
    }
  }

  dynamic changing([ Boolean arg(dynamic /* String | Number */ oldValue,
      dynamic /* String | Number */ newValue) ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("changing", arg);
    } else {
      return this.host.jqxKnob("changing");
    }
  }

  dynamic dragEndAngle([ num arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("dragEndAngle", arg);
    } else {
      return this.host.jqxKnob("dragEndAngle");
    }
  }

  dynamic dragStartAngle([ num arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("dragStartAngle", arg);
    } else {
      return this.host.jqxKnob("dragStartAngle");
    }
  }

  dynamic disabled([ bool arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("disabled", arg);
    } else {
      return this.host.jqxKnob("disabled");
    }
  }

  dynamic dial([ jqwidgets.KnobDial arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("dial", arg);
    } else {
      return this.host.jqxKnob("dial");
    }
  }

  dynamic endAngle([ num arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("endAngle", arg);
    } else {
      return this.host.jqxKnob("endAngle");
    }
  }

  dynamic height([ dynamic /* String | Number */ arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("height", arg);
    } else {
      return this.host.jqxKnob("height");
    }
  }

  dynamic labels([ jqwidgets.KnobLabels arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("labels", arg);
    } else {
      return this.host.jqxKnob("labels");
    }
  }

  dynamic marks([ jqwidgets.KnobMarks arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("marks", arg);
    } else {
      return this.host.jqxKnob("marks");
    }
  }

  dynamic min([ num arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("min", arg);
    } else {
      return this.host.jqxKnob("min");
    }
  }

  dynamic max([ num arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("max", arg);
    } else {
      return this.host.jqxKnob("max");
    }
  }

  dynamic progressBar([ jqwidgets.KnobProgressBar arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("progressBar", arg);
    } else {
      return this.host.jqxKnob("progressBar");
    }
  }

  dynamic pointer([ jqwidgets.KnobPointer arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("pointer", arg);
    } else {
      return this.host.jqxKnob("pointer");
    }
  }

  dynamic pointerGrabAction([ String arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("pointerGrabAction", arg);
    } else {
      return this.host.jqxKnob("pointerGrabAction");
    }
  }

  dynamic rotation([ String arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("rotation", arg);
    } else {
      return this.host.jqxKnob("rotation");
    }
  }

  dynamic startAngle([ num arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("startAngle", arg);
    } else {
      return this.host.jqxKnob("startAngle");
    }
  }

  dynamic spinner([ jqwidgets.KnobSpinner arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("spinner", arg);
    } else {
      return this.host.jqxKnob("spinner");
    }
  }

  dynamic style([ jqwidgets.KnobStyle arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("style", arg);
    } else {
      return this.host.jqxKnob("style");
    }
  }

  dynamic step([ num arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("step", arg);
    } else {
      return this.host.jqxKnob("step");
    }
  }

  dynamic snapToStep([ bool arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("snapToStep", arg);
    } else {
      return this.host.jqxKnob("snapToStep");
    }
  }

  dynamic value([ num arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("value", arg);
    } else {
      return this.host.jqxKnob("value");
    }
  }

  dynamic width([ dynamic /* String | Number */ arg ]) {
    if (!identical(arg, undefined)) {
      this.host.jqxKnob("width", arg);
    } else {
      return this.host.jqxKnob("width");
    }
  }

  // jqxKnobComponent functions
  void destroy() {
    this.host.jqxKnob("destroy");
  }

  dynamic val([ dynamic /* String | Number */ value ]) {
    if (!identical(value, undefined)) {
      this.host.jqxKnob("val", value);
    } else {
      return this.host.jqxKnob("val");
    }
  }

  // jqxKnobComponent events
  @Output () var onChange = new EventEmitter ();

  void ___wireEvents__() {
    this.host.on("change", (dynamic eventData) {
      this.onChange.emit(eventData);
    });
  }
}

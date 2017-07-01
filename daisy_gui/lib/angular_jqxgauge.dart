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
    selector: "jqxGauge",
    template: "<div><ng-content></ng-content></div>"
)
class jqxGaugeComponent implements OnChanges {
  @Input("animationDuration") dynamic attrAnimationDuration;
  @Input("border") dynamic attrBorder;
  @Input("caption") dynamic attrCaption;
  @Input("cap") dynamic attrCap;
  @Input("colorScheme") dynamic attrColorScheme;
  @Input("disabled") dynamic attrDisabled;
  @Input("easing") dynamic attrEasing;
  @Input("endAngle") dynamic attrEndAngle;
  @Input("int64") dynamic attrInt64;
  @Input("labels") dynamic attrLabels;
  @Input("min") dynamic attrMin;
  @Input("max") dynamic attrMax;
  @Input("pointer") dynamic attrPointer;
  @Input("radius") dynamic attrRadius;
  @Input("ranges") dynamic attrRanges;
  @Input("startAngle") dynamic attrStartAngle;
  @Input("showRanges") dynamic attrShowRanges;
  @Input("style") dynamic attrStyle;
  @Input("ticksMajor") dynamic attrTicksMajor;
  @Input("ticksMinor") dynamic attrTicksMinor;
  @Input("ticksDistance") dynamic attrTicksDistance;
  @Input("value") dynamic attrValue;
  @Input("width") dynamic attrWidth;
  @Input("height") dynamic attrHeight;
  @Input("auto-create") bool autoCreate = true;
  List<String> properties = [
    "animationDuration", "border", "caption", "cap", "colorScheme",
    "disabled", "easing", "endAngle", "height", "int64", "labels", "min",
    "max", "pointer", "radius", "ranges", "startAngle", "showRanges",
    "style", "ticksMajor", "ticksMinor", "ticksDistance", "value", "width"
  ];
  dynamic host;
  ElementRef elementRef;
  jqwidgets.jqxGauge widgetObject;

  jqxGaugeComponent(ElementRef containerElement) {
    this.elementRef = containerElement;
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
            this.properties[i].substring(0, 1).toUpperCase() +
            this.properties[i].substring(1);
        bool areEqual;
        if (!identical(this[attrName], undefined)) {
          if (identical(this[attrname], "object")) {
            if (this[attrName] is Array) {
              areEqual = this.arraysEqual(this[attrName],
                  this.host.jqxGauge(this.properties[i]));
            }
            if (areEqual) {
              return false;
            }
            this.host.jqxGauge(this.properties[i], this[attrName]);
            continue;
          }
          if (!identical(this[attrName],
              this.host.jqxGauge(this.properties[i]))) {
            this.host.jqxGauge(this.properties[i], this[attrName]);
          }
        }
      } // end for //
    }
  }

  bool arraysEqual(dynamic attrValue, dynamic hostValue) {
    if (attrValue.length != hostValue.length) {
      return false;
    }
    for (var i = 0; i < attrValue.length; i ++) {
      if (!identical(attrValue[i], hostValue[i])) {
        return false;
      }
    } // end for //
    return true;
  }

  dynamic manageAttributes() {
    var options = {};
    for (var i = 0; i < this.properties.length; i ++) {
      var attrName = "attr" +
          this.properties[i].substring(0, 1).toUpperCase() +
          this.properties[i].substring(1);
      if (!identical(this[attrName], undefined)) {
        options [this.properties[i]] = this[attrName];
      }
    } // end for //
    return options;
  }

  void createComponent([dynamic options]) {
    if (options) {
      JQXLite.extend(options, this.manageAttributes());
    } else {
      options = this.manageAttributes();
    }
    this.host = JQXLite(this.elementRef.nativeElement.firstChild);
    this.___wireEvents__();
    this.widgetObject = jqwidgets.createInstance(
        this.host, "jqxGauge", options);
    this.___updateRect__();
  }

  void createWidget([dynamic options]) {
    this.createComponent(options);
  }

  void ___updateRect__() {
    this.host.css(width: this.attrWidth, height: this.attrHeight);
  }

  void setOptions(dynamic options) {
    this.host.jqxGauge("setOptions", options);
  }

  // jqxGaugeComponent properties
  dynamic animationDuration([dynamic /* String | Number */ arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("animationDuration", arg);
    } else {
      return this.host.jqxGauge("animationDuration");
    }
  }

  dynamic border([jqwidgets.GaugeBorder arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("border", arg);
    } else {
      return this.host.jqxGauge("border");
    }
  }

  dynamic caption([jqwidgets.GaugeCaption arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("caption", arg);
    } else {
      return this.host.jqxGauge("caption");
    }
  }

  dynamic cap([jqwidgets.GaugeCap arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("cap", arg);
    } else {
      return this.host.jqxGauge("cap");
    }
  }

  dynamic colorScheme([String arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("colorScheme", arg);
    } else {
      return this.host.jqxGauge("colorScheme");
    }
  }

  dynamic disabled([bool arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("disabled", arg);
    } else {
      return this.host.jqxGauge("disabled");
    }
  }

  dynamic easing([String arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("easing", arg);
    } else {
      return this.host.jqxGauge("easing");
    }
  }

  dynamic endAngle([dynamic /* String | Number */ arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("endAngle", arg);
    } else {
      return this.host.jqxGauge("endAngle");
    }
  }

  dynamic height([dynamic /* String | Number */ arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("height", arg);
    } else {
      return this.host.jqxGauge("height");
    }
  }

  dynamic int64([bool arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("int64", arg);
    } else {
      return this.host.jqxGauge("int64");
    }
  }

  dynamic labels([jqwidgets.GaugeLabels arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("labels", arg);
    } else {
      return this.host.jqxGauge("labels");
    }
  }

  dynamic min([num arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("min", arg);
    } else {
      return this.host.jqxGauge("min");
    }
  }

  dynamic max([dynamic /* String | Number */ arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("max", arg);
    } else {
      return this.host.jqxGauge("max");
    }
  }

  dynamic pointer([jqwidgets.GaugePointer arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("pointer", arg);
    } else {
      return this.host.jqxGauge("pointer");
    }
  }

  dynamic radius([dynamic /* String | Number */ arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("radius", arg);
    } else {
      return this.host.jqxGauge("radius");
    }
  }

  dynamic ranges([Array<jqwidgets.GaugeRanges> arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("ranges", arg);
    } else {
      return this.host.jqxGauge("ranges");
    }
  }

  dynamic startAngle([dynamic /* String | Number */ arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("startAngle", arg);
    } else {
      return this.host.jqxGauge("startAngle");
    }
  }

  dynamic showRanges([bool arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("showRanges", arg);
    } else {
      return this.host.jqxGauge("showRanges");
    }
  }

  dynamic style([jqwidgets.GaugeStyle arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("style", arg);
    } else {
      return this.host.jqxGauge("style");
    }
  }

  dynamic ticksMajor([jqwidgets.GaugeTicks arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("ticksMajor", arg);
    } else {
      return this.host.jqxGauge("ticksMajor");
    }
  }

  dynamic ticksMinor([jqwidgets.GaugeTicks arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("ticksMinor", arg);
    } else {
      return this.host.jqxGauge("ticksMinor");
    }
  }

  dynamic ticksDistance([dynamic /* String | Number */ arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("ticksDistance", arg);
    } else {
      return this.host.jqxGauge("ticksDistance");
    }
  }

  dynamic value([num arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("value", arg);
    } else {
      return this.host.jqxGauge("value");
    }
  }

  dynamic width([dynamic /* String | Number */ arg]) {
    if (!identical(arg, undefined)) {
      this.host.jqxGauge("width", arg);
    } else {
      return this.host.jqxGauge("width");
    }
  }

  // jqxGaugeComponent functions
  void disable() {
    this.host.jqxGauge("disable");
  }

  void enable() {
    this.host.jqxGauge("enable");
  }

  dynamic val([num value]) {
    if (!identical(value, undefined)) {
      this.host.jqxGauge("val", value);
    } else {
      return this.host.jqxGauge("val");
    }
  }

  // jqxGaugeComponent events
  @Output() var onValueChanging = new EventEmitter();
  @Output() var onValueChanged = new EventEmitter();

  void ___wireEvents__() {
    this.host.on("valueChanging", (dynamic eventData) {
      this.onValueChanging.emit(eventData);
    });
    this.host.on("valueChanged", (dynamic eventData) {
      this.onValueChanged.emit(eventData);
    });
  }

} // end class //

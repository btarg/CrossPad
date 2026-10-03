export type ButtonName =
  | 'START'
  | 'BACK'
  | 'LEFT_THUMB'
  | 'RIGHT_THUMB'
  | 'LEFT_SHOULDER'
  | 'RIGHT_SHOULDER'
  | 'GUIDE'
  | 'A'
  | 'B'
  | 'X'
  | 'Y';

export type AxisName =
  | 'leftX'
  | 'leftY'
  | 'rightX'
  | 'rightY'
  | 'leftTrigger'
  | 'rightTrigger'
  | 'dpadHorz'
  | 'dpadVert';

export type UpdateMode = 'auto' | 'manual';

export interface ControllerInput<T> {
  readonly name: string;
  value: T;
  setValue(value: T): void;
}

export type ButtonInput = ControllerInput<boolean>;
export type AxisInput = ControllerInput<number>;

export type ButtonInputs = {
  [K in ButtonName]: ButtonInput;
};

export type AxisInputs = {
  [K in AxisName]: AxisInput;
};

export class X360Controller {
  readonly button: ButtonInputs;
  readonly axis: AxisInputs;
  updateMode: UpdateMode;

  connect(): null;
  disconnect(): null;
  update(): void;
  resetInputs(): void;
}

export function createX360Controller(): X360Controller;
export function createXboxOneController(): X360Controller;
export function installDriver(): void;

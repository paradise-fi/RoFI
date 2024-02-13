import dataclasses
import json
import types
import typing

from os import PathLike
from typing import Any, TypeVar, Union


Path = Union[str, PathLike[str]]
T = TypeVar("T")
OneOrMore = Union[list[T], T]


class EnhancedJSONEncoder(json.JSONEncoder):
    def default(self, o: Any) -> Any:
        if dataclasses.is_dataclass(o):
            return dataclasses.asdict(o)
        return super().default(o)


def parse_json(cls: type[T], json_value: Any) -> T:
    if dataclasses.is_dataclass(cls):
        return parse_json_dataclass(cls, json_value)

    if typing.get_origin(cls) is not None:
        return parse_generic_json(cls, json_value)

    if cls is Any:
        return json_value

    if not isinstance(cls, type):
        raise NotImplementedError(
            f"Parsing {cls} (of type {type(cls)}) not yet implemented (be sure to update forward references)"
        )
    if not isinstance(json_value, cls):
        raise ValueError(
            f"Cannot parse type {cls} from type {type(json_value)} (value {json_value})"
        )
    return json_value


def parse_generic_json(cls: type[T], json_value: Any) -> T:
    cls_origin = typing.get_origin(cls)
    cls_args = typing.get_args(cls)
    assert cls_origin is not None, f"Type {cls} is not generic"

    if cls_origin is dict:
        if not isinstance(json_value, dict):
            raise ValueError(f"Cannot parse {cls} from type {type(json_value)}")
        if len(cls_args) == 0:
            assert isinstance(json_value, cls), f"Type {cls} is dict with no type args"
            return json_value
        key_cls, value_cls = cls_args
        try:
            return {
                parse_json(key_cls, key): parse_json(value_cls, value)
                for key, value in json_value.items()
            }  # type: ignore
        except ValueError as e:
            raise ValueError(f"Error parsing {cls}: {e}") from e

    if cls_origin is list:
        if not isinstance(json_value, list):
            raise ValueError(f"Cannot parse {cls} from type {type(json_value)}")
        if len(cls_args) == 0:
            assert isinstance(json_value, cls), f"Type {cls} is list with no type args"
            return json_value
        (arg_cls,) = cls_args
        try:
            return [parse_json(arg_cls, value) for value in json_value]  # type: ignore
        except ValueError as e:
            raise ValueError(f"Error parsing {cls}: {e}") from e

    if cls_origin is types.UnionType or cls_origin is Union:
        for arg_cls in cls_args:
            try:
                return parse_json(arg_cls, json_value)
            except ValueError:
                pass
        raise ValueError(f"Cannot parse {cls} from type {type(json_value)}")

    if cls_origin is typing.Literal:
        for arg in cls_args:
            try:
                value = parse_json(type(arg), json_value)
                if value == arg:
                    return value
            except ValueError:
                pass
        raise ValueError(f"Cannot parse {cls} from {json_value}")

    raise NotImplementedError(
        f"Parsing {cls} (of type {type(cls)}) not yet implemented"
    )


def parse_json_dataclass(cls: type[T], json_value: Any) -> T:
    assert dataclasses.is_dataclass(cls), f"Type {cls} is not a dataclass"
    if not isinstance(json_value, dict):
        raise ValueError(f"Cannot parse dataclass {cls} from type {type(json_value)}")

    for key in json_value.keys():
        if not isinstance(key, str):
            raise ValueError(f"Cannot parse dataclass key from type {type(key)}")

    field_types = {field.name: field.type for field in dataclasses.fields(cls)}

    def parse_field(json_key: str, json_value: Any):
        try:
            if json_key in field_types:
                return parse_json(field_types[json_key], json_value)
            else:
                return json_value
        except ValueError as e:
            raise ValueError(
                f"Error parsing field {json_key!r} from {cls.__name__}: {e}"
            ) from e

    field_values = {key: parse_field(key, value) for key, value in json_value.items()}

    result = cls(**field_values)
    assert isinstance(result, cls)
    return result

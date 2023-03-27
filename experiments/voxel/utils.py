import dataclasses
import json
from os import PathLike
import types
from typing import Any, Dict, List, Union
import typing


Path = Union[str, PathLike[str]]


class EnhancedJSONEncoder(json.JSONEncoder):
    def default(self, o: Any) -> Any:
        if dataclasses.is_dataclass(o):
            return dataclasses.asdict(o)
        return super().default(o)


def parse_json(cls: type, json_value: Any) -> Any:
    if dataclasses.is_dataclass(cls):
        return parse_json_dataclass(cls, json_value)

    if typing.get_origin(cls) is not None:
        return parse_generic_json(cls, json_value)

    if cls is Any:
        return json_value

    if not isinstance(cls, type):
        raise NotImplementedError(
            f"Parsing type {cls} from type {type(json_value)} is not yet implemented"
        )
    if not isinstance(json_value, cls):
        raise ValueError(f"Cannot parse type {cls} from type {type(json_value)}")
    return json_value


def parse_generic_json(cls: type, json_value: Any) -> Any:
    cls_origin = typing.get_origin(cls)
    cls_args = typing.get_args(cls)
    assert cls_origin is not None

    if cls_origin is dict:
        if not isinstance(json_value, dict):
            raise ValueError(f"Cannot parse {cls} from type {type(json_value)}")
        json_dict: Dict[Any, Any] = json_value
        if len(cls_args) == 0:
            return json_dict
        key_cls, value_cls = cls_args
        return {
            parse_json(key_cls, key): parse_json(value_cls, value)
            for key, value in json_dict.items()
        }

    if cls_origin is list:
        if not isinstance(json_value, list):
            raise ValueError(f"Cannot parse {cls} from type {type(json_value)}")
        json_list: List[Any] = json_value
        if len(cls_args) == 0:
            return json_list
        (arg_cls,) = cls_args
        return [parse_json(arg_cls, value) for value in json_list]

    if cls_origin is types.UnionType or cls_origin is Union:
        for arg_cls in cls_args:
            try:
                return parse_json(arg_cls, json_value)
            except ValueError:
                pass
        raise ValueError(f"Cannot parse {cls} from type {type(json_value)}")

    if not isinstance(cls_origin, type):
        raise NotImplementedError(
            f"Parsing {cls} (of type {type(cls)}) from type {type(json_value)} not yet implemented"
        )

    raise NotImplementedError(
        f"Parsing {cls} (of type {type(cls)}) from type {type(json_value)} not yet implemented"
    )


def parse_json_dataclass(cls: type, json_value: Any) -> Any:
    assert dataclasses.is_dataclass(cls)
    if not isinstance(json_value, dict):
        raise ValueError(f"Cannot parse dataclass {cls} from type {type(json_value)}")

    field_values: Dict[str, Any] = json_value
    assert all(isinstance(name, str) for name in field_values)

    field_types = {field.name: field.type for field in dataclasses.fields(cls)}
    field_values = {
        name: parse_json(field_types[name], value) if name in field_types else value
        for name, value in field_values.items()
    }

    result = cls(**field_values)
    assert isinstance(result, cls)
    return result
